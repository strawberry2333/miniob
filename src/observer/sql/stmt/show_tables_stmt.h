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
// Created by Wangyunlai on 2023/6/14.
//

#pragma once

#include <string>
#include <vector>

#include "sql/stmt/stmt.h"

class Db;

/**
 * @file show_tables_stmt.h
 * @brief 定义 `SHOW TABLES` 的语义对象。
 */

/**
 * @brief 表示显示当前数据库所有表的语句。
 * @ingroup Statement
 * @details 当前语句本身无额外载荷，执行时直接从当前 DB 枚举表列表。
 */
class ShowTablesStmt : public Stmt
{
public:
  ShowTablesStmt()          = default;
  virtual ~ShowTablesStmt() = default;

  StmtType type() const override { return StmtType::SHOW_TABLES; }

  /**
   * @brief 创建一个无状态的 `ShowTablesStmt`。
   */
  static RC create(Db *db, Stmt *&stmt)
  {
    stmt = new ShowTablesStmt();
    return RC::SUCCESS;
  }
};
