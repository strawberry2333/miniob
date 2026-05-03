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
// Created by WangYunlai on 2022/6/27.
//

#pragma once

#include "sql/expr/expression.h"
#include "sql/operator/physical_operator.h"

class FilterStmt;

/**
 * @file predicate_physical_operator.h
 * @brief 行式过滤算子。
 */

/**
 * @brief 过滤/谓词物理算子
 * @ingroup PhysicalOperator
 */
class PredicatePhysicalOperator : public PhysicalOperator
{
public:
  PredicatePhysicalOperator(unique_ptr<Expression> expr);

  virtual ~PredicatePhysicalOperator() = default;

  PhysicalOperatorType type() const override { return PhysicalOperatorType::PREDICATE; }
  OpType               get_op_type() const override { return OpType::FILTER; }

  /// @brief 打开唯一子算子并准备按行过滤。
  RC open(Trx *trx) override;
  /// @brief 循环拉取子 tuple，直到遇到谓词为真的一行或子算子耗尽。
  RC next() override;
  RC close() override;

  Tuple *current_tuple() override;

  RC tuple_schema(TupleSchema &schema) const override;

private:
  unique_ptr<Expression> expression_;
};
