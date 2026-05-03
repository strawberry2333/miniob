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
// Created by Wangyunlai on 2022/6/5.
//

#pragma once

#include "common/sys/rc.h"
#include "sql/expr/expression.h"
#include "sql/stmt/stmt.h"

class Db;
class Table;

/**
 * @file calc_stmt.h
 * @brief 定义 `CALC` 语句的语义对象。
 */

/**
 * @brief 表示只计算表达式结果的 `CALC` 语句。
 * @ingroup Statement
 */
class CalcStmt : public Stmt
{
public:
  CalcStmt()                   = default;
  virtual ~CalcStmt() override = default;

  StmtType type() const override { return StmtType::CALC; }

public:
  /**
   * @brief 从 parse 阶段的表达式列表构造 `CalcStmt`。
   * @param calc_sql parse 阶段语法节点，会把表达式所有权移动进 stmt。
   * @param stmt 输出语句对象。
   * @return 总是返回 `RC::SUCCESS`。
   */
  static RC create(CalcSqlNode &calc_sql, Stmt *&stmt)
  {
    CalcStmt *calc_stmt     = new CalcStmt();
    // `CALC` 不需要额外 schema 绑定，直接接管 parse 阶段构造的表达式树。
    calc_stmt->expressions_ = std::move(calc_sql.expressions);
    stmt                    = calc_stmt;
    return RC::SUCCESS;
  }

public:
  /// 返回需要计算的表达式列表。
  vector<unique_ptr<Expression>> &expressions() { return expressions_; }

private:
  vector<unique_ptr<Expression>> expressions_;  ///< 按输出顺序保存的表达式树
};
