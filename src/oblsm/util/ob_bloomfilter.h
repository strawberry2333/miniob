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

#include "common/lang/string.h"
namespace oceanbase {

/**
 * @class ObBloomfilter
 * @brief Bloom Filter 接口占位。
 *
 * Bloom Filter 的典型职责是在读路径上快速回答：
 * “某个 key 是否一定不存在于当前 SSTable / Block 中”。
 *
 * 标准实现通常包含：
 * - 一个位图；
 * - 多个哈希函数；
 * - `insert()` 时把多个位置置 1；
 * - `contains()` 时检查这些位是否都为 1。
 *
 * 语义上它只能保证：
 * - 返回 `false` 时，元素一定不存在；
 * - 返回 `true` 时，元素“可能存在”，允许误判。
 *
 * 当前代码仍是接口骨架：没有真正保存位图，也没有任何哈希或并发控制逻辑。
 * 因此这些方法只是为了固定调用面，不能提供真实过滤能力。
 */
class ObBloomfilter
{
public:
  /**
   * @brief 构造一个 Bloom Filter 描述对象。
   *
   * @param hash_func_count 计划使用的哈希函数个数。
   * @param totoal_bits 计划使用的位图总 bit 数。
   *
   * 这两个参数描述的是“理想实现”的过滤器规模；
   * 当前占位实现不会真正保存它们，也不会据此分配位图。
   */
  ObBloomfilter(size_t hash_func_count = 4, size_t totoal_bits = 65536) {}

  /**
   * @brief 向过滤器中登记一个对象。
   *
   * 在完整实现里，这一步会对 `object` 做多次哈希，并把对应 bit 位置为 1。
   * 当前版本是空操作，仅保留接口形态。
   *
   * @param object 需要登记的对象字节串。
   */
  void insert(const string &object) {}

  /**
   * @brief 清空过滤器。
   *
   * 理想情况下这里应把位图全部清零，并重置插入计数。
   * 当前版本没有内部状态，因此同样是空操作。
   */
  void clear() {}

  /**
   * @brief 判断对象是否“可能存在”。
   *
   * 注意：当前实现恒返回 `false`，表示它还没有真正承担过滤职责。
   * 调用方不能依赖它做存在性剪枝。
   *
   * @param object 待检查对象。
   * @return 在完整实现里，`true` 表示可能存在，`false` 表示一定不存在。
   */
  bool contains(const string &object) const { return false; }

  /**
   * @brief 返回已登记对象数量。
   *
   * 完整实现通常会维护一个逻辑计数，便于估算误判率或调试观察。
   * 当前版本恒返回 0。
   */
  size_t object_count() const { return 0; }

  /**
   * @brief 判断过滤器是否为空。
   * @return 当 `object_count()` 为 0 时返回 `true`。
   */
  bool empty() const { return 0 == object_count(); }

private:
  // 当前占位实现没有成员状态；后续若补齐功能，通常会在这里放置位图和统计信息。
};

}  // namespace oceanbase
