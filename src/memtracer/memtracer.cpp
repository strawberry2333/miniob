/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "memtracer.h"
#include <dlfcn.h>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <chrono>
#include <execinfo.h>
#include <iostream>
#include <cstdio>

#define REACH_TIME_INTERVAL(i)                                                                                \
  ({                                                                                                          \
    bool                    bret      = false;                                                                \
    static volatile int64_t last_time = 0;                                                                    \
    auto                    now       = std::chrono::system_clock::now();                                     \
    int64_t cur_time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count(); \
    if (int64_t(i + last_time) < cur_time) [[unlikely]] {                                                     \
      last_time = cur_time;                                                                                   \
      bret      = true;                                                                                       \
    }                                                                                                         \
    bret;                                                                                                     \
  })
extern memtracer::malloc_func_t orig_malloc;
extern memtracer::free_func_t   orig_free;
extern memtracer::mmap_func_t   orig_mmap;
extern memtracer::munmap_func_t orig_munmap;

namespace memtracer {

// 从 /proc/self/status 的某一行中解析内存大小，返回值单位为 KB。
// 格式示例："VmExe:   1234 kB"，跳过字段名，直接读取数值。
long get_memory_size(const std::string &line)
{
  std::string        token;
  std::istringstream iss(line);
  long               size;
  iss >> token;  // 跳过字段名（如 "VmExe:"）
  iss >> size;
  return size;  // 单位：KB
}

MemTracer &MemTracer::get_instance()
{
  // 单例实现：利用局部静态变量的线程安全初始化（C++11 保证）
  static MemTracer instance;
  return instance;
}

void MemTracer::init()
{
  // 初始化 malloc/free/mmap/munmap 的 hook，使后续所有分配都经过 MemTracer 统计
  MT.init_hook_funcs();

  // 从环境变量 MT_MEMORY_LIMIT 读取内存上限（字节），超过后进程主动退出
  const char *memory_limit_str = std::getenv("MT_MEMORY_LIMIT");
  if (memory_limit_str != nullptr) {
    char              *end;
    unsigned long long value = std::strtoull(memory_limit_str, &end, 10);
    if (end != memory_limit_str && *end == '\0') {
      MT.set_memory_limit(static_cast<size_t>(value));
    } else {
      MEMTRACER_LOG("Invalid environment variable value for MT_MEMORY_LIMIT: %s\n", memory_limit_str);
    }
  }

  // 从环境变量 MT_PRINT_INTERVAL_MS 读取统计日志的打印间隔（毫秒），默认 5 秒
  const char *print_interval_ms_str = std::getenv("MT_PRINT_INTERVAL_MS");
  if (print_interval_ms_str != nullptr) {
    char              *end;
    unsigned long long value = std::strtoull(print_interval_ms_str, &end, 10);
    if (end != print_interval_ms_str && *end == '\0') {
      MT.set_print_interval(static_cast<size_t>(value));
    } else {
      MEMTRACER_LOG("Invalid environment variable value for MT_MEMORY_LIMIT: %s\n", print_interval_ms_str);
    }
  } else {
    MT.set_print_interval(1000 * 5);  // 默认 5 秒打印一次
  }

  // 读取进程代码段（text segment）的内存占用并计入已分配内存。
  // VmExe 对应 /proc/self/status 中可执行代码段的大小。
  // TODO: 暂不支持统计全局/静态变量占用的内存（BSS/data 段）。
  size_t        text_size = 0;
  std::ifstream file("/proc/self/status");
  if (file.is_open()) {
    std::string line;
    const int   KB = 1024;
    while (std::getline(file, line)) {
      if (line.find("VmExe") != std::string::npos) {
        text_size = get_memory_size(line) * KB;
        break;
      }
    }
    file.close();
  }

  // 将代码段大小作为初始内存基线加入统计
  MT.add_allocated_memory(text_size);
  // 启动后台统计线程，定期打印内存使用情况
  MT.init_stats_thread();
}

void MemTracer::init_stats_thread()
{
  // 避免重复启动：只有线程未运行时才创建
  if (!t_.joinable()) {
    t_ = std::thread(stat);
  }
}

void MemTracer::alloc(size_t size)
{
  // fetch_add 返回旧值，加上 size 后与限制比较，超限则打印日志并强制退出。
  // [[unlikely]] 提示编译器超限是罕见路径，优化正常路径的分支预测。
  if (allocated_memory_.fetch_add(size) + size > memory_limit_) [[unlikely]] {
    MEMTRACER_LOG("alloc memory:%lu, allocated_memory: %lu, memory_limit: %lu, Memory limit exceeded!\n",
        size,
        allocated_memory_.load(),
        memory_limit_);
    exit(-1);
  }
  alloc_cnt_.fetch_add(1);
}

void MemTracer::free(size_t size)
{
  // 释放时减少已分配计数，并增加 free 次数统计
  allocated_memory_.fetch_sub(size);
  free_cnt_.fetch_add(1);
}

// 进程退出时由 atexit 或析构调用，确保统计线程正常退出
void MemTracer::destroy() { MT.stop(); }

void MemTracer::stop()
{
  is_stop_ = true;
  t_.join();  // 等待统计线程退出，避免在析构后访问已销毁的成员
}

void MemTracer::init_hook_funcs_impl()
{
  // 使用 dlsym(RTLD_NEXT, ...) 获取 libc 中原始函数的地址。
  // RTLD_NEXT 表示在当前库之后的动态库中查找符号，
  // 这样 hook 函数可以在拦截后调用真正的 malloc/free/mmap/munmap。
  orig_malloc = (void *(*)(size_t size))dlsym(RTLD_NEXT, "malloc");
  orig_free   = (void (*)(void *ptr))dlsym(RTLD_NEXT, "free");
  orig_mmap = (void *(*)(void *addr, size_t length, int prot, int flags, int fd, off_t offset))dlsym(RTLD_NEXT, "mmap");
  orig_munmap = (int (*)(void *addr, size_t length))dlsym(RTLD_NEXT, "munmap");
}

void MemTracer::stat()
{
  const size_t print_interval_ms = MT.print_interval();
  // 内部以 100ms 为粒度轮询，避免线程长期阻塞无法响应 is_stop_ 信号
  const size_t sleep_interval = 100;  // 单位：毫秒
  while (!MT.is_stop()) {
    // REACH_TIME_INTERVAL 使用静态时间戳实现节流，只有达到打印间隔才输出日志
    if (REACH_TIME_INTERVAL(print_interval_ms)) {
      // TODO: 优化输出格式，支持更友好的单位换算（KB/MB/GB）
      MEMTRACER_LOG("allocated memory: %lu, metadata memory: %lu\n", MT.allocated_memory(), MT.meta_memory());
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_interval));
  }
}
}  // namespace memtracer