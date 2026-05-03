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
// Created by Wangyunlai on 2022/12/07.
//

#pragma once

#include "sql/operator/logical_operator.h"

/**
 * @file calc_logical_operator.h
 * @brief `SELECT 1 + 2` 这类无表表达式查询的逻辑算子。
 */

/**
 * @brief 纯表达式计算逻辑算子。
 * @details 该算子不依赖输入表，执行时会把 `expressions_` 直接包装成一行输出。
 */
class CalcLogicalOperator : public LogicalOperator
{
public:
  CalcLogicalOperator(vector<unique_ptr<Expression>> &&expressions) { expressions_.swap(expressions); }
  virtual ~CalcLogicalOperator() = default;

  LogicalOperatorType type() const override { return LogicalOperatorType::CALC; }
  OpType              get_op_type() const override { return OpType::LOGICALCALCULATE; }
};
