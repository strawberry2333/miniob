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
//
// 迭代器是整个 oblsm 模块最重要的抽象之一。
// MemTable、Block、SSTable、MergingIterator、UserIterator 都实现了这套接口，
// 这样上层可以把“不同来源的数据”统一看成一条有序流。
//
// 线程安全约定与 LevelDB 类似：
// - 只读接口在对象生命周期受保护的前提下可以并发调用；
// - 一旦有线程调用了 seek/next 之类的非 const 方法，就需要外部自行加锁。

#pragma once

#include "common/lang/string_view.h"
#include "common/sys/rc.h"

namespace oceanbase {

/**
 * @class ObLsmIterator
 * @brief ObLsm 中所有有序遍历器的统一抽象接口。
 *
 * 这套接口刻意保持很小，只保留：
 * - 定位：`seek` / `seek_to_first` / `seek_to_last`
 * - 前进：`next`
 * - 取值：`key` / `value`
 * - 状态：`valid`
 *
 * 这样可以让上层自由组合不同来源的数据迭代器，最终构建出合并读、范围读、
 * 版本过滤读等更复杂的访问路径。
 */
class ObLsmIterator
{
public:
  ObLsmIterator(){};

  ObLsmIterator(const ObLsmIterator &) = delete;

  ObLsmIterator &operator=(const ObLsmIterator &) = delete;

  virtual ~ObLsmIterator(){};

  /**
   * @brief 当前游标是否指向一个有效元素。
   *
   * 只有在 `valid() == true` 时，`key()` / `value()` 才是可访问的。
   */
  virtual bool valid() const = 0;

  /**
   * @brief 将游标推进到下一个元素。
   */
  virtual void next() = 0;

  /**
   * @brief 返回当前位置的 key。
   *
   * 注意：具体返回的是 user key 还是 internal key，取决于迭代器实现。
   * 例如：
   * - Block/SSTable/MergingIterator 往往处理 internal key；
   * - UserIterator 会把它转换成 user key 视图。
   */
  virtual string_view key() const = 0;

  /**
   * @brief 返回当前位置的 value。
   */
  virtual string_view value() const = 0;

  /**
   * @brief 将游标定位到第一个 `>= k` 的元素。
   *
   * 这里的比较语义同样取决于具体迭代器使用的 key 格式和 comparator。
   */
  virtual void seek(const string_view &k) = 0;

  /**
   * @brief 定位到数据源中的第一个元素。
   */
  virtual void seek_to_first() = 0;

  /**
   * @brief 定位到数据源中的最后一个元素。
   */
  virtual void seek_to_last() = 0;
};

}  // namespace oceanbase
