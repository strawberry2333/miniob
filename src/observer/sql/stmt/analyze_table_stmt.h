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

#include "sql/stmt/stmt.h"

class Db;

/**
 * @file analyze_table_stmt.h
 * @brief 定义 `ANALYZE TABLE` 对应的语义语句对象。
 */

/**
 * @brief 表示 `ANALYZE TABLE` 语句。
 * @ingroup Statement
 */
class AnalyzeTableStmt : public Stmt
{
public:
  /**
   * @brief 以目标表名构造分析表语句。
   * @param table_name 需要统计的表名。
   */
  AnalyzeTableStmt(const string &table_name) : table_name_(table_name) {}
  virtual ~AnalyzeTableStmt() = default;

  StmtType type() const override { return StmtType::ANALYZE_TABLE; }

  /// 返回需要执行统计的表名。
  const string &table_name() const { return table_name_; }

  /**
   * @brief 从 parse 节点构造 `AnalyzeTableStmt`。
   * @param db 当前数据库。
   * @param analyze_table parse 阶段产出的语法节点。
   * @param stmt 输出语句对象。
   * @return 表存在则返回成功，否则返回 schema 错误。
   */
  static RC create(Db *db, const AnalyzeTableSqlNode &analyze_table, Stmt *&stmt);

private:
  string table_name_;  ///< 目标表名
};
