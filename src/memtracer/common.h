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

#include <stdio.h>

namespace memtracer {

// mt_visible 将符号标记为动态库默认可见，使其可被外部（如 LD_PRELOAD 加载方）访问。
// 若不加此属性，编译器可能将符号优化为局部可见，导致 hook 失效。
#define mt_visible __attribute__((visibility("default")))

// 原始 libc 函数指针类型定义。
// init_hook_funcs_impl 通过 dlsym(RTLD_NEXT, ...) 获取这些函数的真实地址，
// 在 hook 函数内部调用原始实现，避免无限递归。
using malloc_func_t = void *(*)(size_t);
using free_func_t   = void (*)(void *);
using mmap_func_t   = void *(*)(void *, size_t, int, int, int, off_t);
using munmap_func_t = int (*)(void *, size_t);

void log_stderr(const char *format, ...);

// 统一的日志宏，所有 memtracer 内部日志都加 "[MEMTRACER]" 前缀输出到 stderr，
// 与业务日志区分，方便过滤。使用 do-while(0) 包裹保证宏在各种语法场景下安全展开。
#define MEMTRACER_LOG(format, ...)     \
  do {                                 \
    fprintf(stderr, "[MEMTRACER] ");   \
    log_stderr(format, ##__VA_ARGS__); \
  } while (0)

}  // namespace memtracer