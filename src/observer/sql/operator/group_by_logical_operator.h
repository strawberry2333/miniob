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
// Created by WangYunlai on 2024/05/29.
//

#pragma once

#include "sql/operator/logical_operator.h"

/**
 * @file group_by_logical_operator.h
 * @brief `GROUP BY` 逻辑算子。
 */

/**
 * @brief 描述分组列和聚合输出列的逻辑节点。
 * @details `LogicalPlanGenerator::create_group_by_plan` 会把查询表达式中扫描到的
 * `AggregateExpr` 和显式 `GROUP BY` 列交给它统一保存。
 */
class GroupByLogicalOperator : public LogicalOperator
{
public:
  /// @brief 接管分组列表达式和聚合表达式指针集合。
  GroupByLogicalOperator(vector<unique_ptr<Expression>> &&group_by_exprs, vector<Expression *> &&expressions);

  virtual ~GroupByLogicalOperator() = default;

  LogicalOperatorType type() const override { return LogicalOperatorType::GROUP_BY; }
  OpType              get_op_type() const override { return OpType::LOGICALGROUPBY; }

  auto &group_by_expressions() { return group_by_expressions_; }
  auto &aggregate_expressions() { return aggregate_expressions_; }

private:
  vector<unique_ptr<Expression>> group_by_expressions_;
  vector<Expression *>           aggregate_expressions_;  ///< 输出的表达式，可能包含聚合函数
};
