/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "sql/stmt/analyze_table_stmt.h"
#include "storage/db/db.h"

/**
 * @file analyze_table_stmt.cpp
 * @brief 实现 `ANALYZE TABLE` 的语义绑定。
 */

RC AnalyzeTableStmt::create(Db *db, const AnalyzeTableSqlNode &analyze_table, Stmt *&stmt)
{
  // resolve 阶段先确认目标表存在，执行阶段就可以专注于统计逻辑。
  if (db->find_table(analyze_table.relation_name.c_str()) == nullptr) {
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }
  stmt = new AnalyzeTableStmt(analyze_table.relation_name);
  return RC::SUCCESS;
}
