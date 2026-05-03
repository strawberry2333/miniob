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

#include "common/sys/rc.h"
#include "common/lang/memory.h"

class LogicalOperator;
class Expression;

/**
 * @file rewrite_rule.h
 * @brief 逻辑计划与表达式重写规则接口。
 */

/**
 * @brief 逻辑计划的重写规则
 * @ingroup Rewriter
 * TODO: 重构下当前的查询改写规则，放到 cascade optimizer 中。
 */
class RewriteRule
{
public:
  virtual ~RewriteRule() = default;

  /**
   * @brief 对一个逻辑算子子树应用规则。
   * @param oper 当前待改写的根节点。
   * @param change_made 若规则真正改动了树结构或表达式，则置为 `true`。
   */
  virtual RC rewrite(unique_ptr<LogicalOperator> &oper, bool &change_made) = 0;
};

/**
 * @brief 表达式的重写规则
 * @ingroup Rewriter
 */
class ExpressionRewriteRule
{
public:
  virtual ~ExpressionRewriteRule() = default;

  /// @brief 对单个表达式节点尝试做局部改写。
  virtual RC rewrite(unique_ptr<Expression> &expr, bool &change_made) = 0;
};
