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
// Created by WangYunlai on 2024/05/30.
//

#include "common/log/log.h"
#include "sql/operator/group_by_logical_operator.h"
#include "sql/expr/expression.h"

using namespace std;

/**
 * @file group_by_logical_operator.cpp
 * @brief `GROUP BY` 逻辑节点的构造实现。
 */

GroupByLogicalOperator::GroupByLogicalOperator(vector<unique_ptr<Expression>> &&group_by_exprs,
                                               vector<Expression *> &&expressions)
{
  // 分组列表达式拥有所有权，聚合表达式只保留在查询表达式树中的裸指针视图。
  group_by_expressions_ = std::move(group_by_exprs);
  aggregate_expressions_ = std::move(expressions);
}
