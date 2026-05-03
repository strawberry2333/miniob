/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#pragma once

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

/**
 * @file arena_allocator.h
 * @brief 提供按块分配、整体释放的简单 arena 分配器。
 * @details 该实现来自 LevelDB 风格的 bump allocator。调用方只能追加申请内存，
 * 不支持逐对象释放；真正的资源回收发生在 Arena 析构时一次性完成。
 */

/**
 * @brief 基于块的线性分配器。
 * @details 适合大量小对象、生命周期与容器一致的场景，例如向量化执行中的变长字符串缓存。
 * 资源管理约束如下：
 * 1. `Allocate*` 返回的内存由 Arena 统一托管，调用方不能单独 `delete/free`。
 * 2. 当前实现默认由单线程顺序使用，只有 `memory_usage_` 统计使用了原子变量。
 * 3. 空间不足时会自动扩容新块；底层 `new[]` 失败会抛出异常，而不是返回 `RC`。
 */
class Arena
{
public:
  Arena();

  Arena(const Arena &)            = delete;
  Arena &operator=(const Arena &) = delete;

  ~Arena();

  /**
   * @brief 申请一段连续内存。
   * @param bytes 申请字节数，必须大于 0。
   * @return 返回可写内存首地址。
   * @details 优先从当前块顺序切分；如果剩余空间不足，则回退到 `AllocateFallback` 新开块。
   */
  char *Allocate(size_t bytes);

  /**
   * @brief 申请满足指针对齐要求的内存。
   * @details 当当前块中无法同时满足容量和对齐要求时，会退化为 `AllocateFallback`。
   * 该接口适合需要放置原生对象或 SIMD 友好数据的场景。
   */
  char *AllocateAligned(size_t bytes);

  /**
   * @brief 返回 arena 当前累计占用的近似字节数。
   * @details 该值包含每个块本身和块指针的管理开销，用于观测内存增长趋势，而不是精确计费。
   */
  size_t MemoryUsage() const { return memory_usage_.load(std::memory_order_relaxed); }

private:
  /**
   * @brief 当当前块空间不足时的兜底分配路径。
   * @details 大对象会直接独立成块，小对象会触发标准大小块的补充，避免大量尾部碎片。
   */
  char *AllocateFallback(size_t bytes);
  /**
   * @brief 真正向系统申请一块新内存并纳入 arena 生命周期管理。
   */
  char *AllocateNewBlock(size_t block_bytes);

  /// 当前块中的下一个可分配地址。
  char  *alloc_ptr_;
  /// 当前块还剩余多少字节可直接切分。
  size_t alloc_bytes_remaining_;

  /// Arena 持有的所有底层块；析构时统一遍历释放。
  std::vector<char *> blocks_;

  /// Arena 申请过的总内存统计。
  ///
  /// TODO(costan): 这里仅统计变量使用原子操作，但其它状态并未并发保护。
  std::atomic<size_t> memory_usage_;
};

inline char *Arena::Allocate(size_t bytes)
{
  // The semantics of what to return are a bit messy if we allow
  // 0-byte allocations, so we disallow them here (we don't need
  // them for our internal use).
  assert(bytes > 0);
  if (bytes <= alloc_bytes_remaining_) {
    char *result = alloc_ptr_;
    alloc_ptr_ += bytes;
    alloc_bytes_remaining_ -= bytes;
    return result;
  }
  return AllocateFallback(bytes);
}
