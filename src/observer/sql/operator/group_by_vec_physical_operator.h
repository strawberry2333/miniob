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

#include "sql/expr/aggregate_hash_table.h"
#include "sql/operator/physical_operator.h"

/**
 * @file group_by_vec_physical_operator.h
 * @brief 向量化分组聚合算子占位声明。
 * @details 当前接口已经接入计划生成器，但实现仍返回 `RC::UNIMPLEMENTED`。
 */

/**
 * @brief Group By 物理算子(vectorized)
 * @ingroup PhysicalOperator
 */
class GroupByVecPhysicalOperator : public PhysicalOperator
{
public:
  /// @brief 预留接管分组列表达式与聚合表达式的构造入口。
  GroupByVecPhysicalOperator(vector<unique_ptr<Expression>> &&group_by_exprs, vector<Expression *> &&expressions){};

  virtual ~GroupByVecPhysicalOperator() = default;

  PhysicalOperatorType type() const override { return PhysicalOperatorType::GROUP_BY_VEC; }

  RC open(Trx *trx) override { return RC::UNIMPLEMENTED; }
  RC next(Chunk &chunk) override { return RC::UNIMPLEMENTED; }
  RC close() override { return RC::UNIMPLEMENTED; }

private:
};
