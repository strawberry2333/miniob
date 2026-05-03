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

#include "sql/operator/physical_operator.h"

/**
 * @file expr_vec_physical_operator.h
 * @brief 向量化表达式求值算子。
 * @details 该算子位于 `ProjectVecPhysicalOperator` 下方，负责把输入 chunk 中的原始列
 * 转换成投影/聚合所需的表达式结果列。
 */

/**
 * @brief 表达式物理算子(Vectorized)
 * @ingroup PhysicalOperator
 */
class ExprVecPhysicalOperator : public PhysicalOperator
{
public:
  ExprVecPhysicalOperator(vector<Expression *> &&expressions);

  virtual ~ExprVecPhysicalOperator() = default;

  PhysicalOperatorType type() const override { return PhysicalOperatorType::EXPR_VEC; }

  RC open(Trx *trx) override;
  /// @brief 对当前输入 chunk 的每个表达式生成一列结果。
  RC next(Chunk &chunk) override;
  RC close() override;

private:
  vector<Expression *> expressions_;  /// 表达式
  Chunk                chunk_;
  Chunk                evaled_chunk_;
};
