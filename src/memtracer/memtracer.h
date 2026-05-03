/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#pragma once

#include <cstdlib>

#include "memtracer/common.h"
#include "common/lang/thread.h"
#include "common/lang/atomic.h"
#include "common/lang/mutex.h"

namespace memtracer {

// 全局单例访问快捷宏，等价于 MemTracer::get_instance()
#define MT MemTracer::get_instance()

/**
 * @brief 内存追踪器，用于监控 MiniOB 进程的内存使用情况。
 *
 * MemTracer 通过替换（hook）系统的 malloc/free/mmap/munmap 函数，
 * 拦截所有内存分配和释放操作，实时统计已分配内存总量。
 * 主要用于在内存受限环境下运行和调试 MiniOB，当内存超过上限时主动退出。
 *
 * 工作原理：
 * 1. 动态库加载时，`__attribute__((constructor))` 触发 init()，完成 hook 初始化。
 * 2. 之后所有 malloc/free 调用都经过 allocator.cpp 中的 hook 函数，
 *    hook 函数在调用原始 libc 函数的同时，通知 MemTracer 更新计数。
 * 3. 后台统计线程定期打印当前内存使用量。
 * 4. 进程退出时，`__attribute__((destructor))` 触发 destroy()，停止统计线程。
 *
 * 更多细节参见 docs/src/game/miniob-memtracer.md
 */
class MemTracer
{
public:
  // 单例模式，全局唯一实例，通过 MT 宏访问
  static MemTracer &get_instance();

  MemTracer() = default;

  // 动态库加载完成后自动调用，完成 hook 函数注册和后台线程启动
  static void __attribute__((constructor)) init();

  // 进程退出前自动调用，确保统计线程正常退出
  static void __attribute__((destructor)) destroy();

  // 当前已分配的内存总量（字节），包含代码段基线和所有 malloc/mmap 分配
  size_t allocated_memory() const { return allocated_memory_.load(); }

  // 元数据内存：每次 malloc 会在用户数据前额外存一个 size_t 记录大小，
  // 未释放的分配数乘以 sizeof(size_t) 就是当前元数据占用量
  size_t meta_memory() const { return ((alloc_cnt_.load() - free_cnt_.load()) * sizeof(size_t)); }

  size_t print_interval() const { return print_interval_ms_; }

  size_t memory_limit() const { return memory_limit_; }

  bool is_stop() const { return is_stop_; }

  void set_print_interval(size_t print_interval_ms) { print_interval_ms_ = print_interval_ms; }

  inline void add_allocated_memory(size_t size) { allocated_memory_.fetch_add(size); }

  // 内存上限只允许设置一次，call_once 保证多线程并发调用时的安全性
  void set_memory_limit(size_t memory_limit)
  {
    call_once(memory_limit_once_, [&]() { memory_limit_ = memory_limit; });
  }

  // 记录一次分配，超过内存上限时打印日志并 exit(-1)
  void alloc(size_t size);

  // 记录一次释放，从已分配计数中减去对应大小
  void free(size_t size);

  // hook 初始化也只执行一次，call_once 保证线程安全；
  // allocator.cpp 中每个 hook 函数入口都会调用此方法，
  // 因为 hook 可能在 init() 执行前就被触发（如全局对象构造时的 malloc）
  inline void init_hook_funcs() { call_once(init_hook_funcs_once_, init_hook_funcs_impl); }

private:
  // 通过 dlsym(RTLD_NEXT, ...) 获取 libc 原始函数地址
  static void init_hook_funcs_impl();

  // 启动后台统计线程（只启动一次）
  void init_stats_thread();

  // 设置停止标志并等待统计线程退出
  void stop();

  // 后台统计线程函数，定期打印内存使用情况
  static void stat();

private:
  bool           is_stop_ = false;
  atomic<size_t> allocated_memory_{};    // 当前已分配内存总量（字节）
  atomic<size_t> alloc_cnt_{};           // 累计 alloc 次数，用于计算元数据内存
  atomic<size_t> free_cnt_{};            // 累计 free 次数，用于计算元数据内存
  once_flag      init_hook_funcs_once_;  // 保证 hook 初始化只执行一次
  once_flag      memory_limit_once_;     // 保证内存上限只被设置一次
  size_t         memory_limit_      = UINT64_MAX;  // 内存上限，默认不限制
  size_t         print_interval_ms_ = 0;           // 统计日志打印间隔（毫秒）
  thread         t_;                               // 后台统计线程
};
}  // namespace memtracer