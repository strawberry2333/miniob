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
#include "sql/stmt/stmt.h"
#include "storage/field/field.h"

class FieldMeta;
class FilterStmt;
class Db;
class Table;

/**
 * @file select_stmt.h
 * @brief 定义 `SELECT` 的语义对象。
 */

/**
 * @brief 表示一条完成表和表达式绑定的 `SELECT` 语句。
 * @ingroup Statement
 */
class SelectStmt : public Stmt
{
public:
  SelectStmt() = default;
  ~SelectStmt() override;

  StmtType type() const override { return StmtType::SELECT; }

public:
  /**
   * @brief 从 parse 节点构造 `SelectStmt`。
   * @param db 当前数据库。
   * @param select_sql parse 阶段 select 节点。
   * @param stmt 输出语句对象。
   * @return 返回表绑定、表达式绑定和 where 构造结果。
   */
  static RC create(Db *db, SelectSqlNode &select_sql, Stmt *&stmt);

public:
  /// 返回 FROM 子句绑定出的表列表。
  const vector<Table *> &tables() const { return tables_; }
  /// 返回 WHERE 子句绑定结果。
  FilterStmt            *filter_stmt() const { return filter_stmt_; }

  /// 返回 SELECT 列表绑定后的表达式数组。
  vector<unique_ptr<Expression>> &query_expressions() { return query_expressions_; }
  /// 返回 GROUP BY 绑定后的表达式数组。
  vector<unique_ptr<Expression>> &group_by() { return group_by_; }

private:
  vector<unique_ptr<Expression>> query_expressions_;  ///< SELECT 输出表达式
  vector<Table *>                tables_;             ///< FROM 子句绑定出的表
  FilterStmt                    *filter_stmt_ = nullptr; ///< WHERE 子句过滤条件
  vector<unique_ptr<Expression>> group_by_;           ///< GROUP BY 表达式
};
