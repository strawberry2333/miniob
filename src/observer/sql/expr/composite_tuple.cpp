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

#include "sql/expr/composite_tuple.h"
#include "common/log/log.h"

using namespace std;

/**
 * @file composite_tuple.cpp
 * @brief `CompositeTuple` 的顺序展开实现。
 */

int CompositeTuple::cell_num() const
{
  int num = 0;
  for (const auto &tuple : tuples_) {
    num += tuple->cell_num();
  }
  return num;
}

RC CompositeTuple::cell_at(int index, Value &cell) const
{
  for (const auto &tuple : tuples_) {
    if (index < tuple->cell_num()) {
      // 每个子 tuple 都被视作一段连续区间，命中后直接委托给它处理局部下标。
      return tuple->cell_at(index, cell);
    } else {
      index -= tuple->cell_num();
    }
  }
  return RC::NOTFOUND;
}

RC CompositeTuple::spec_at(int index, TupleCellSpec &spec) const
{
  for (const auto &tuple : tuples_) {
    if (index < tuple->cell_num()) {
      return tuple->spec_at(index, spec);
    } else {
      index -= tuple->cell_num();
    }
  }
  return RC::NOTFOUND;
}

RC CompositeTuple::find_cell(const TupleCellSpec &spec, Value &cell) const
{
  RC rc = RC::NOTFOUND;
  for (const auto &tuple : tuples_) {
    rc = tuple->find_cell(spec, cell);
    if (OB_SUCC(rc)) {
      // 找到首个匹配列后立即返回，保持与 join 结果“左侧优先”的查找语义一致。
      break;
    }
  }
  return rc;
}

void CompositeTuple::add_tuple(unique_ptr<Tuple> tuple) { tuples_.push_back(std::move(tuple)); }

Tuple &CompositeTuple::tuple_at(size_t index) 
{ 
  ASSERT(index < tuples_.size(), "index=%d, tuples_size=%d", index, tuples_.size());
  return *tuples_[index]; 
}
