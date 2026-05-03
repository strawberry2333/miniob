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
// Created by Wangyunlai on 2022/12/27.
//

#pragma once

#include "sql/stmt/stmt.h"

/**
 * @file explain_stmt.h
 * @brief 定义 `EXPLAIN` 语句的语义对象。
 */

/**
 * @brief 表示 `EXPLAIN <command>` 语句。
 * @ingroup Statement
 */
class ExplainStmt : public Stmt
{
public:
  /**
   * @brief 构造 explain 语句。
   * @param child_stmt 被 explain 的内部语句。
   */
  ExplainStmt(unique_ptr<Stmt> child_stmt);
  virtual ~ExplainStmt() = default;

  StmtType type() const override { return StmtType::EXPLAIN; }

  /// 返回被 explain 的内部语句。
  Stmt *child() const { return child_stmt_.get(); }

  /**
   * @brief 先递归构造内部语句，再包装成 `ExplainStmt`。
   */
  static RC create(Db *db, const ExplainSqlNode &query, Stmt *&stmt);

private:
  unique_ptr<Stmt> child_stmt_;  ///< 被 explain 的真实语句对象
};
