/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include <sys/mman.h>  // mmap/munmap
#include <string.h>

#include "memtracer/allocator.h"

// dlsym 内部会调用 calloc，而 calloc hook 又需要调用 dlsym 获取原始函数，
// 会造成无限递归。用一个静态 buffer 作为 calloc 的临时返回值，
// 在 orig_malloc 尚未初始化时兜底，跳出死锁。
// 参考：https://stackoverflow.com/questions/7910666/problems-with-ld-preload-and-calloc-interposition-for-certain-executables
static unsigned char calloc_buffer[8192];

// 读取 malloc 返回给用户的指针前一个 size_t 位置存储的原始分配大小。
// memtracer 的 malloc 在用户数据前额外分配 sizeof(size_t) 字节用于存储大小，
// free 时通过此函数取回大小，通知 MemTracer 减去对应计数。
inline size_t ptr_size(void *ptr)
{
  if (ptr == NULL) [[unlikely]] {
    return 0;
  }
  return *((size_t *)ptr - 1);
}

// 内存布局示意：
//   [ size_t: 用户请求的字节数 | 用户数据区 ]
//   ^orig_malloc 返回          ^返回给用户的指针
mt_visible void *malloc(size_t size)
{
  MT.init_hook_funcs();
  size_t *ptr = (size_t *)orig_malloc(size + sizeof(size_t));
  if (ptr == NULL) [[unlikely]] {
    return NULL;
  }
  *ptr = size;  // 在 header 中记录用户请求的大小，free 时用于还原计数
  MT.alloc(size);
  return (void *)(ptr + 1);  // 返回跳过 header 后的用户数据起始地址
}

// calloc 语义：分配 nelem*size 字节并清零。
// 特殊情况：orig_malloc 为 NULL 说明 hook 尚未初始化（dlsym 初始化期间），
// 返回静态 buffer 避免死锁，此时内存不计入统计。
mt_visible void *calloc(size_t nelem, size_t size)
{
  if (orig_malloc == NULL) [[unlikely]] {
    return calloc_buffer;
  }
  size_t alloc_size = nelem * size;
  void  *ptr        = malloc(alloc_size);
  if (ptr == NULL) [[unlikely]] {
    return NULL;
  }
  memset(ptr, 0, alloc_size);
  return ptr;
}

// realloc 语义：扩大或缩小已有分配块。
// 当新大小超过旧大小时，分配新块并拷贝旧数据，再释放旧块；
// 否则原地返回（不缩容），避免不必要的拷贝。
// 注意：标准 realloc 可能原地扩展，这里简化为总是新建块，语义正确但略有性能差异。
mt_visible void *realloc(void *ptr, size_t size)
{
  if (ptr == NULL) {
    return malloc(size);
  }
  void *res = NULL;
  if (ptr_size(ptr) < size) {
    res = malloc(size);
    if (res == NULL) [[unlikely]] {
      return NULL;
    }
    memcpy(res, ptr, ptr_size(ptr));
    free(ptr);
  } else {
    res = ptr;
  }
  return res;
}

mt_visible void free(void *ptr)
{
  MT.init_hook_funcs();
  // calloc_buffer 是静态栈内存，不需要也不能传给 orig_free
  if (ptr == NULL || ptr == calloc_buffer) [[unlikely]] {
    return;
  }
  MT.free(ptr_size(ptr));
  orig_free((size_t *)ptr - 1);  // 传给 orig_free 的是含 header 的原始指针
}

// cfree 是 glibc 的历史遗留接口，等同于 free
mt_visible void cfree(void *ptr) { free(ptr); }

// mmap 分配匿名内存（flags 含 MAP_ANONYMOUS）时也纳入统计，
// 例如 glibc 对大块内存（>= MMAP_THRESHOLD，通常 128KB）直接用 mmap 分配。
mt_visible void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
  MT.init_hook_funcs();
  void *res = orig_mmap(addr, length, prot, flags, fd, offset);
  if (res != MAP_FAILED) [[likely]] {
    MT.alloc(length);
  }
  return res;
}

mt_visible int munmap(void *addr, size_t length)
{
  MT.init_hook_funcs();
  int res = orig_munmap(addr, length);
  if (res == 0) [[likely]] {
    MT.free(length);
  }
  return res;
}

// strdup/strndup 通过我们的 malloc hook 分配内存，因此自动计入统计
mt_visible char *strdup(const char *s) MT_THROW
{
  size_t len = strlen(s);
  char  *p   = (char *)malloc(len + 1);
  if (p == NULL) {
    return NULL;
  }
  memcpy(p, s, len);
  p[len] = 0;
  return p;
}

mt_visible char *strndup(const char *s, size_t n) MT_THROW
{
  const char  *end = (const char *)memchr(s, 0, n);
  const size_t m   = (end != NULL ? (size_t)(end - s) : n);
  char        *t   = (char *)malloc(m + 1);
  if (t == NULL)
    return NULL;
  memcpy(t, s, m);
  t[m] = 0;
  return t;
}

// 以下函数因内存布局与 memtracer header 方案不兼容而不支持，调用时直接退出。
// realpath 在某些实现中内部自行管理缓冲区大小，难以统一 hook。
mt_visible char *realpath(const char *fname, char *resolved_name)
{
  MEMTRACER_LOG("realpath not supported\n");
  exit(-1);
}

// memalign/valloc/pvalloc/posix_memalign 要求返回地址满足特定对齐，
// 而 memtracer 在指针前插入 size_t header 会破坏对齐保证，暂不支持。
mt_visible void *memalign(size_t alignment, size_t size)
{
  MEMTRACER_LOG("memalign not supported\n");
  exit(-1);
}

mt_visible void *valloc(size_t size)
{
  MEMTRACER_LOG("valloc not supported\n");
  exit(-1);
}

mt_visible void *pvalloc(size_t size)
{
  MEMTRACER_LOG("pvalloc not supported\n");
  exit(-1);
}

mt_visible int posix_memalign(void **memptr, size_t alignment, size_t size)
{
  MEMTRACER_LOG("posix_memalign not supported\n");
  exit(-1);
}

// brk/sbrk 通过移动堆顶指针直接分配内存，无法插入 header 记录大小，不支持。
#ifdef LINUX
mt_visible int brk(void *addr)
{
  MEMTRACER_LOG("brk not supported\n");
  exit(-1);
}

mt_visible void *sbrk(intptr_t increment)
{
  MEMTRACER_LOG("sbrk not supported\n");
  exit(-1);
}

mt_visible long int syscall(long int __sysno, ...)
{
  MEMTRACER_LOG("syscall not supported\n");
  exit(-1);
}
#elif defined(__MACH__)
mt_visible void *brk(const void *addr)
{
  MEMTRACER_LOG("brk not supported\n");
  exit(-1);
}
mt_visible void *sbrk(int increment)
{
  MEMTRACER_LOG("sbrk not supported\n");
  exit(-1);
}
mt_visible int syscall(int __sysno, ...)
{
  MEMTRACER_LOG("syscall not supported\n");
  exit(-1);
}
#endif

// C++ new/delete 全部转发到 malloc/free hook，确保对象分配也被统计
mt_visible void *operator new(std::size_t size) { return malloc(size); }

mt_visible void *operator new[](std::size_t size) { return malloc(size); }

mt_visible void *operator new(std::size_t size, const std::nothrow_t &) noexcept { return malloc(size); }

mt_visible void *operator new[](std::size_t size, const std::nothrow_t &) noexcept { return malloc(size); }

mt_visible void operator delete(void *ptr) noexcept { free(ptr); }

mt_visible void operator delete[](void *ptr) noexcept { free(ptr); }

mt_visible void operator delete(void *ptr, const std::nothrow_t &) noexcept { free(ptr); }

mt_visible void operator delete[](void *ptr, const std::nothrow_t &) noexcept { free(ptr); }

// sized delete：C++14 引入，编译器可能传入对象大小，这里忽略 size 直接转发给 free
mt_visible void operator delete(void *ptr, std::size_t size) noexcept { free(ptr); }

mt_visible void operator delete[](void *ptr, std::size_t size) noexcept { free(ptr); }