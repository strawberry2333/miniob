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

#include "sql/stmt/desc_table_stmt.h"
#include "storage/db/db.h"

/**
 * @file desc_table_stmt.cpp
 * @brief 实现 `DESC` 语句的语义绑定。
 */

RC DescTableStmt::create(Db *db, const DescTableSqlNode &desc_table, Stmt *&stmt)
{
  // 提前检查表是否存在，执行阶段可以直接进入结果集构造。
  if (db->find_table(desc_table.relation_name.c_str()) == nullptr) {
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }
  stmt = new DescTableStmt(desc_table.relation_name);
  return RC::SUCCESS;
}
