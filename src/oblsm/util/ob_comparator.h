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

#include "common/lang/string_view.h"

namespace oceanbase {

/**
 * @brief 比较器抽象基类。
 *
 * 整个 oblsm 非常依赖“有序性”：
 * - MemTable 的跳表需要比较器；
 * - Block / SSTable 的查找与遍历需要比较器；
 * - 多路归并读和 compaction 也需要比较器。
 *
 * 因此比较器不仅决定“谁排在前面”，还会间接决定：
 * - 同一个 user key 的多个版本在迭代流里的出现顺序；
 * - seek 的命中行为；
 * - compaction 去重时优先保留哪一个版本。
 */
class ObComparator
{
public:
  virtual ~ObComparator() = default;

  /**
   * @brief 三路比较。
   *
   * 调用方需要保证比较规则是稳定且自反的，否则跳表、二分查找和归并逻辑
   * 都可能被破坏。
   *
   * @return `< 0` 表示 `a < b`
   * @return `== 0` 表示 `a == b`
   * @return `> 0` 表示 `a > b`
   */
  virtual int compare(const string_view &a, const string_view &b) const = 0;
};

/**
 * @brief 默认的用户键比较器，直接按字节字典序比较。
 *
 * 它不理解序列号、删除标记等高级语义，只负责比较原始 user key。
 */
class ObDefaultComparator : public ObComparator
{
public:
  explicit ObDefaultComparator() = default;
  int compare(const string_view &a, const string_view &b) const override;
};

/**
 * @brief internal key 比较器。
 *
 * internal key 的真实含义是：
 * `user_key + sequence_number`
 *
 * 比较规则分两步：
 * 1. 先按 user key 正常字典序比较；
 * 2. 若 user key 相同，再按 seq 倒序比较。
 *
 * 之所以对 seq 使用“倒序”，是因为我们希望同一个 user key 的最新版本
 * 在有序流里排在前面，这样无论是点查还是迭代去重都更容易实现。
 *
 * 这个比较器是 oblsm 中最关键的顺序规则之一：只要输入是合法 internal key，
 * 上层就能在合并多个有序流时优先看到最新版本。
 */
class ObInternalKeyComparator : public ObComparator
{
public:
  explicit ObInternalKeyComparator() = default;

  // `a`/`b` 都应满足 internal key 格式：`user_key + seq(8B)`。
  int                 compare(const string_view &a, const string_view &b) const override;
  const ObComparator *user_comparator() const { return &default_comparator_; }

private:
  ObDefaultComparator default_comparator_;
};

}  // namespace oceanbase
