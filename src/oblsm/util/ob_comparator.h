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
 */
class ObComparator
{
public:
  virtual ~ObComparator() = default;

  /**
   * @brief 三路比较。
   * @return `< 0` 表示 `a < b`
   * @return `== 0` 表示 `a == b`
   * @return `> 0` 表示 `a > b`
   */
  virtual int compare(const string_view &a, const string_view &b) const = 0;
};

/**
 * @brief 默认的用户键比较器，直接按字典序比较。
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
 */
class ObInternalKeyComparator : public ObComparator
{
public:
  explicit ObInternalKeyComparator() = default;

  int                 compare(const string_view &a, const string_view &b) const override;
  const ObComparator *user_comparator() const { return &default_comparator_; }

private:
  ObDefaultComparator default_comparator_;
};

}  // namespace oceanbase
