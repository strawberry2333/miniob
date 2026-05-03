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
// Created by Wangyunlai on 2022/5/27.
//

#pragma once

#include "sql/parser/parse_defs.h"
#include "sql/stmt/stmt.h"

class Table;
class FilterStmt;

/**
 * @file delete_stmt.h
 * @brief 定义 `DELETE` 的语义对象。
 */

/**
 * @brief 表示 `DELETE FROM ... WHERE ...` 语句。
 * @ingroup Statement
 */
class DeleteStmt : public Stmt
{
public:
  /**
   * @brief 构造删除语句。
   * @param table 目标表。
   * @param filter_stmt 绑定后的过滤条件，可为空。
   */
  DeleteStmt(Table *table, FilterStmt *filter_stmt);
  ~DeleteStmt() override;

  /// 返回目标表。
  Table      *table() const { return table_; }
  /// 返回过滤条件。
  FilterStmt *filter_stmt() const { return filter_stmt_; }

  StmtType type() const override { return StmtType::DELETE; }

public:
  /**
   * @brief 从 parse 节点构造删除语句，并绑定 where 条件。
   */
  static RC create(Db *db, const DeleteSqlNode &delete_sql, Stmt *&stmt);

private:
  Table      *table_       = nullptr;  ///< 删除目标表
  FilterStmt *filter_stmt_ = nullptr;  ///< WHERE 子句绑定结果
};
