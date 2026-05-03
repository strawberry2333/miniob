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
// Created by Wangyunlai on 2022/5/22.
//

#pragma once

#include "common/sys/rc.h"
#include "sql/stmt/stmt.h"

class Table;
class Db;

/**
 * @file insert_stmt.h
 * @brief 定义 `INSERT` 的语义对象。
 */

/**
 * @brief 表示一条单行 `INSERT INTO ... VALUES (...)` 语句。
 * @ingroup Statement
 */
class InsertStmt : public Stmt
{
public:
  InsertStmt() = default;
  /**
   * @brief 构造插入语句。
   * @param table 目标表。
   * @param values 需要插入的值数组。
   * @param value_amount 值数量。
   */
  InsertStmt(Table *table, const Value *values, int value_amount);

  StmtType type() const override { return StmtType::INSERT; }

public:
  /**
   * @brief 从 parse 节点构造插入语句，并检查目标表和字段个数。
   */
  static RC create(Db *db, const InsertSqlNode &insert_sql, Stmt *&stmt);

public:
  /// 返回目标表。
  Table       *table() const { return table_; }
  /// 返回待插入值数组。
  const Value *values() const { return values_; }
  /// 返回待插入值个数。
  int          value_amount() const { return value_amount_; }

private:
  Table       *table_        = nullptr;  ///< 目标表
  const Value *values_       = nullptr;  ///< 待插入值数组
  int          value_amount_ = 0;        ///< 待插入值个数
};
