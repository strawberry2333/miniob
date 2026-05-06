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

#include <stdint.h>
#include <cstddef>

namespace oceanbase {

/**
 * @class ObLRUCache
 * @brief LRU 缓存接口占位。
 *
 * 按设计，它应当提供一个固定容量的最近最少使用缓存，常见用途包括：
 * - block cache；
 * - 元数据缓存；
 * - 读路径上的热点对象复用。
 *
 * 完整实现通常会同时维护：
 * - 哈希表：按 key O(1) 查找；
 * - 双向链表：维护最近访问顺序；
 * - 必要的锁：保证并发访问安全。
 *
 * 当前版本尚未落地这些数据结构，所有接口都只是骨架：
 * - `get()` 恒返回 miss；
 * - `put()` 不保存数据；
 * - `contains()` 恒返回不存在。
 *
 * @tparam KeyType 缓存键类型。
 * @tparam ValueType 缓存值类型。
 */
template <typename KeyType, typename ValueType>
class ObLRUCache
{
public:
  /**
   * @brief 创建一个容量受限的缓存描述对象。
   *
   * @param capacity 理论最大容量。
   *
   * 当前实现只保存这个数字，不会真正分配缓存结构。
   */
  ObLRUCache(size_t capacity) : capacity_(capacity) {}

  /**
   * @brief 按 key 查询缓存。
   *
   * 理想情况下，命中后还应把条目移动到“最近访问”位置。
   * 当前版本没有内部状态，因此始终返回未命中。
   *
   * @param key 待查询键。
   * @param value 若命中，用于承接结果值；当前实现不会写入。
   * @return 当前恒为 `false`。
   */
  bool get(const KeyType &key, ValueType &value) { return false; }

  /**
   * @brief 插入或更新缓存条目。
   *
   * 完整实现里，这里应处理更新、提升访问热度以及超容量淘汰。
   * 当前版本为 no-op，仅保留接口定义。
   *
   * @param key 缓存键。
   * @param value 缓存值。
   */
  void put(const KeyType &key, const ValueType &value) {}

  /**
   * @brief 判断 key 是否存在于缓存中。
   *
   * 当前实现没有保存任何条目，因此恒返回 `false`。
   *
   * @param key 待检查键。
    */
  bool contains(const KeyType &key) const { return false; }

private:
  /**
   * @brief 期望容量上限。
   */
  size_t capacity_;
};

/**
 * @brief 创建一个 LRU 缓存实例。
 *
 * 在完整实现里，这个工厂通常会返回堆上缓存对象，供 block cache 等模块持有。
 * 当前占位版本恒返回 `nullptr`。
 *
 * @tparam Key 键类型。
 * @tparam Value 值类型。
 * @param capacity 理论容量上限。
 * @return 当前恒为 `nullptr`。
 */
template <typename Key, typename Value>
ObLRUCache<Key, Value> *new_lru_cache(uint32_t capacity)
{
  return nullptr;
}

}  // namespace oceanbase
