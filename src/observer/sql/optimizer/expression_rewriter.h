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
// Created by Wangyunlai on 2022/12/13.
//

#pragma once

#include "common/lang/memory.h"
#include "common/sys/rc.h"
#include "sql/expr/expression.h"
#include "sql/operator/logical_operator.h"
#include "sql/optimizer/rewrite_rule.h"

/**
 * @file expression_rewriter.h
 * @brief 表达式级重写器。
 */

/**
 * @brief 在逻辑计划的表达式树上递归应用局部简化规则。
 */
class ExpressionRewriter : public RewriteRule
{
public:
  ExpressionRewriter();
  virtual ~ExpressionRewriter() = default;

  RC rewrite(unique_ptr<LogicalOperator> &oper, bool &change_made) override;

private:
  /// @brief 深度优先改写单个表达式节点及其子树。
  RC rewrite_expression(unique_ptr<Expression> &expr, bool &change_made);

private:
  vector<unique_ptr<ExpressionRewriteRule>> expr_rewrite_rules_;
};
