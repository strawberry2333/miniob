/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Wangyunlai on 2024/5/31.
//

#pragma once

#include "sql/expr/tuple.h"

/**
 * @file composite_tuple.h
 * @brief 把多个 tuple 拼成一个逻辑 tuple 的适配器。
 */

/**
 * @brief 组合的Tuple
 * @ingroup Tuple
 * @details join/group by 在保留原始输入列并拼接额外结果列时都会用到它。
 */
class CompositeTuple : public Tuple
{
public:
  CompositeTuple()          = default;
  virtual ~CompositeTuple() = default;

  /// @brief 删除默认构造函数
  CompositeTuple(const CompositeTuple &) = delete;
  /// @brief 删除默认赋值函数
  CompositeTuple &operator=(const CompositeTuple &) = delete;

  /// @brief 保留移动构造函数
  CompositeTuple(CompositeTuple &&) = default;
  /// @brief 保留移动赋值函数
  CompositeTuple &operator=(CompositeTuple &&) = default;

  /// @brief 返回所有子 tuple 的列数总和。
  int cell_num() const override;
  /// @brief 通过顺序展开的方式定位全局列号对应的子 tuple。
  RC  cell_at(int index, Value &cell) const override;
  RC  spec_at(int index, TupleCellSpec &spec) const override;
  /// @brief 依次在每个子 tuple 中查找指定列。
  RC  find_cell(const TupleCellSpec &spec, Value &cell) const override;

  /// @brief 追加一个拥有所有权的子 tuple。
  void   add_tuple(unique_ptr<Tuple> tuple);
  /// @brief 直接访问某个子 tuple，供调用方复用已缓存的数据。
  Tuple &tuple_at(size_t index);

private:
  vector<unique_ptr<Tuple>> tuples_;
};
