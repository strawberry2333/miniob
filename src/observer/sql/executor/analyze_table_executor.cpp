/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "sql/executor/analyze_table_executor.h"

#include "common/log/log.h"
#include "event/session_event.h"
#include "event/sql_event.h"
#include "session/session.h"
#include "sql/stmt/analyze_table_stmt.h"
#include "storage/db/db.h"
#include "storage/table/table.h"
#include "catalog/catalog.h"
#include "storage/record/record_scanner.h"

using namespace std;

/**
 * @file analyze_table_executor.cpp
 * @brief 实现 `ANALYZE TABLE` 的执行流程。
 */

RC AnalyzeTableExecutor::execute(SQLStageEvent *sql_event)
{
  RC            rc            = RC::SUCCESS;
  Stmt         *stmt          = sql_event->stmt();
  SessionEvent *session_event = sql_event->session_event();
  Session      *session       = session_event->session();
  // 这一层只处理分析表命令，类型不匹配说明上游分派逻辑出错。
  ASSERT(stmt->type() == StmtType::ANALYZE_TABLE,
      "analyze table executor can not run this command: %d",
      static_cast<int>(stmt->type()));

  AnalyzeTableStmt *analyze_table_stmt = static_cast<AnalyzeTableStmt *>(stmt);
  SqlResult        *sql_result         = session_event->sql_result();
  const char       *table_name         = analyze_table_stmt->table_name().c_str();

  // 先在当前数据库里解析目标表。
  Db    *db    = session->get_current_db();
  Table *table = db->find_table(table_name);
  if (table != nullptr) {
    // TODO: optimize the analyze table compute. we can only get table statistics from metadata
    // Don't really scan the whole table!!
    // 当前实现通过逐条扫描记录的方式统计行数，再更新 catalog 中的统计信息。
    int table_id = table->table_id();
    table->get_record_scanner(scanner_, session->current_trx(), ReadWriteMode::READ_ONLY);
    Record dummy;
    int row_nums = 0;
    // 这里只关心记录数量，因此复用一个 dummy 记录对象持续拉取即可。
    while (OB_SUCC(rc = scanner_->next(dummy))) {
      row_nums++;
    }
    // 扫描走到 EOF 代表统计成功完成，不算真正错误。
    if (rc == RC::RECORD_EOF) {
      rc = RC::SUCCESS;
    } else {
      return rc;
    }

    // 将新统计结果写回全局目录，供优化器等后续阶段使用。
    TableStats stats(row_nums);
    Catalog::get_instance().update_table_stats(table_id, stats);
  } else {
    // 命令执行器负责把用户可见错误写回结果对象。
    sql_result->set_return_code(RC::SCHEMA_TABLE_NOT_EXIST);
    sql_result->set_state_string("Table not exists");
  }
  return rc;
}

AnalyzeTableExecutor::~AnalyzeTableExecutor() 
{
  // 扫描器由执行器独占持有，析构时负责兜底释放。
  if (scanner_ != nullptr) {
    delete scanner_;
    scanner_ = nullptr;
  }
}
