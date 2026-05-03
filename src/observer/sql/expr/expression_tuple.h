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

#include "common/lang/vector.h"
#include "sql/expr/tuple.h"
#include "common/value.h"
#include "common/sys/rc.h"

/**
 * @file expression_tuple.h
 * @brief 把一组表达式包装成 `Tuple` 接口的模板适配器。
 * @details 投影、聚合等算子需要把表达式结果伪装成普通 tuple，以便复用
 * 现有的 `Tuple`/`ValueListTuple` 工具链。
 */

template <typename ExprPointerType>
class ExpressionTuple : public Tuple
{
public:
  ExpressionTuple(const vector<ExprPointerType> &expressions) : expressions_(expressions) {}
  virtual ~ExpressionTuple() = default;

  /**
   * @brief 指定表达式求值时依赖的输入 tuple。
   * @details 为空时表示表达式必须可常量折叠，此时会走 `try_get_value`。
   */
  void set_tuple(const Tuple *tuple) { child_tuple_ = tuple; }

  int cell_num() const override { return static_cast<int>(expressions_.size()); }

  RC cell_at(int index, Value &cell) const override
  {
    if (index < 0 || index >= cell_num()) {
      return RC::INVALID_ARGUMENT;
    }

    const ExprPointerType &expression = expressions_[index];
    return get_value(expression, cell);
  }

  RC spec_at(int index, TupleCellSpec &spec) const override
  {
    if (index < 0 || index >= cell_num()) {
      return RC::INVALID_ARGUMENT;
    }

    const ExprPointerType &expression = expressions_[index];
    spec                              = TupleCellSpec(expression->name());
    return RC::SUCCESS;
  }

  RC find_cell(const TupleCellSpec &spec, Value &cell) const override
  {
    RC rc = RC::SUCCESS;
    if (child_tuple_ != nullptr) {
      rc = child_tuple_->find_cell(spec, cell);
      if (OB_SUCC(rc)) {
        return rc;
      }
    }

    rc = RC::NOTFOUND;
    for (const ExprPointerType &expression : expressions_) {
      if (0 == strcmp(spec.alias(), expression->name())) {
        rc = get_value(expression, cell);
        break;
      }
    }

    return rc;
  }

private:
  RC get_value(const ExprPointerType &expression, Value &value) const
  {
    RC rc = RC::SUCCESS;
    if (child_tuple_ != nullptr) {
      // 正常执行路径：基于当前输入 tuple 计算表达式。
      rc = expression->get_value(*child_tuple_, value);
    } else {
      // 纯常量路径：供计划构造或无输入算子直接读取表达式常量值。
      rc = expression->try_get_value(value);
    }
    return rc;
  }

private:
  const vector<ExprPointerType> &expressions_;
  const Tuple                   *child_tuple_ = nullptr;
};
