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

#include <cassert>
#include "common/lang/atomic.h"
#include "common/lang/vector.h"

namespace oceanbase {

/**
 * @brief 极简 Arena 分配器。
 *
 * 这个类提供“只分配、整体释放”的内存生命周期：
 * - 每次 `alloc()` 都直接申请一段新的连续内存；
 * - 调用方拿到指针后不需要也不能单独释放；
 * - 所有内存在 Arena 析构时统一回收。
 *
 * 当前实现没有做 chunk 复用或小块聚合，属于最直接的占位版本。
 * 因此它更像是“统一托管一批 `new[]` 出来的缓冲区”，而不是高性能内存池。
 *
 * @note
 * 1. 线程不安全，调用方需要自行串行化访问；
 * 2. `memory_usage()` 统计的是已申请字节数以及记录块指针的额外开销；
 * 3. 适合和跳表节点、编码后的 key/value 缓冲区一起使用，避免逐条 `delete`。
 */
class ObArena
{
public:
  ObArena();

  ObArena(const ObArena &)            = delete;
  ObArena &operator=(const ObArena &) = delete;

  ~ObArena();

  char *alloc(size_t bytes);

  size_t memory_usage() const { return memory_usage_; }

private:
  // 记录所有通过 `new[]` 申请出来的块，析构时逐块释放。
  vector<char *> blocks_;

  // Arena 当前托管的总内存量，包含数据区和 `blocks_` 中保存指针的估算开销。
  size_t memory_usage_;
};

inline char *ObArena::alloc(size_t bytes)
{
  if (bytes <= 0) {
    return nullptr;
  }
  // 当前实现每次都分配独立块，不会复用之前的剩余空间。
  char *result = new char[bytes];
  blocks_.push_back(result);
  memory_usage_ += bytes + sizeof(char *);
  return result;
}

}  // namespace oceanbase
