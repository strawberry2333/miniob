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

#include "memtracer/common.h"
#include "memtracer/memtracer.h"

using namespace memtracer;

// Linux 上 glibc 的函数声明带有 __THROW 属性（表示不抛 C++ 异常），
// 其他平台没有此属性，用宏统一处理
#if defined(__linux__)
#define MT_THROW __THROW
#else
#define MT_THROW
#endif

// 保存 libc 原始函数指针，在 init_hook_funcs_impl() 中通过 dlsym 初始化。
// 声明为全局变量是因为 hook 函数（malloc/free 等）需要在任意时刻调用原始实现。
malloc_func_t orig_malloc = nullptr;
free_func_t   orig_free   = nullptr;
mmap_func_t   orig_mmap   = nullptr;
munmap_func_t orig_munmap = nullptr;

// ---- 核心内存分配 hook 声明 ----
// 这些函数替换 libc 中的同名符号，所有进程内的分配/释放都会经过这里。
extern "C" mt_visible void *malloc(size_t size);
extern "C" mt_visible void *calloc(size_t nelem, size_t size);
extern "C" mt_visible void *realloc(void *ptr, size_t size);
extern "C" mt_visible void  free(void *ptr);
extern "C" mt_visible void  cfree(void *ptr);
extern "C" mt_visible void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
extern "C" mt_visible int   munmap(void *addr, size_t length);
extern "C" mt_visible char *strdup(const char *s) MT_THROW;
extern "C" mt_visible char *strndup(const char *s, size_t n) MT_THROW;

// ---- C++ operator new/delete hook 声明 ----
// C++ 的 new/delete 底层调用 malloc/free，但编译器可能直接链接到 operator new，
// 因此需要单独 hook，确保 C++ 对象的内存也被统计到。
mt_visible void *operator new(std::size_t size);
mt_visible void *operator new[](std::size_t size);
mt_visible void *operator new(std::size_t size, const std::nothrow_t &) noexcept;
mt_visible void *operator new[](std::size_t size, const std::nothrow_t &) noexcept;
mt_visible void  operator delete(void *ptr) noexcept;
mt_visible void  operator delete[](void *ptr) noexcept;
mt_visible void  operator delete(void *ptr, const std::nothrow_t &) noexcept;
mt_visible void  operator delete[](void *ptr, const std::nothrow_t &) noexcept;
mt_visible void  operator delete(void *ptr, std::size_t size) noexcept;
mt_visible void  operator delete[](void *ptr, std::size_t size) noexcept;

// ---- 不支持的内存分配函数 ----
// 这些函数的内存布局与 memtracer 的 header 方案不兼容（需要对齐等特殊处理），
// 暂不支持，调用时直接报错退出，避免统计数据静默出错。
extern "C" mt_visible char *realpath(const char *fname, char *resolved_name);
extern "C" mt_visible void *memalign(size_t alignment, size_t size);
extern "C" mt_visible void *valloc(size_t size);
extern "C" mt_visible void *pvalloc(size_t size);
extern "C" mt_visible int   posix_memalign(void **memptr, size_t alignment, size_t size);

// brk/sbrk/syscall 是通过移动堆顶指针分配内存的旧式接口，
// memtracer 无法追踪其分配大小，因此禁止使用
#ifdef LINUX
extern "C" mt_visible int      brk(void *addr);
extern "C" mt_visible void    *sbrk(intptr_t increment);
extern "C" mt_visible long int syscall(long int __sysno, ...);
#elif defined(__MACH__)
extern "C" mt_visible void *brk(const void *addr);
extern "C" mt_visible void *sbrk(int increment);
extern "C" mt_visible int   syscall(int __sysno, ...);
#endif

// ---- glibc 内部接口转发（仅 Linux/glibc）----
// glibc 内部某些代码直接调用 __libc_malloc 等内部符号而不经过 malloc，
// 用 alias 属性将这些符号重定向到我们的 hook，确保不遗漏任何分配路径。
#if defined(__GLIBC__) && defined(__linux__)
extern "C" mt_visible void *__libc_malloc(size_t size) __attribute__((alias("malloc"), used));
extern "C" mt_visible void *__libc_calloc(size_t nmemb, size_t size) __attribute__((alias("calloc"), used));
extern "C" mt_visible void *__libc_realloc(void *ptr, size_t size) __attribute__((alias("realloc"), used));
extern "C" mt_visible void  __libc_free(void *ptr) __attribute__((alias("free"), used));
extern "C" mt_visible void  __libc_cfree(void *ptr) __attribute__((alias("cfree"), used));
extern "C" mt_visible void *__libc_valloc(size_t size) __attribute__((alias("valloc"), used));
extern "C" mt_visible void *__libc_pvalloc(size_t size) __attribute__((alias("pvalloc"), used));
extern "C" mt_visible void *__libc_memalign(size_t alignment, size_t size) __attribute__((alias("memalign"), used));
extern "C" mt_visible int   __posix_memalign(void **memptr, size_t alignment, size_t size)
    __attribute__((alias("posix_memalign"), used));
#endif
