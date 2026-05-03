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
// Created by Longda on 2021/4/13.
//

#include <string.h>

#include "resolve_stage.h"

#include "common/conf/ini.h"
#include "common/io/io.h"
#include "common/lang/string.h"
#include "common/log/log.h"
#include "event/session_event.h"
#include "event/sql_event.h"
#include "session/session.h"
#include "sql/stmt/stmt.h"

using namespace common;

/**
 * @file resolve_stage.cpp
 * @brief 实现 parse 节点到 `Stmt` 的语义绑定阶段。
 */

RC ResolveStage::handle_request(SQLStageEvent *sql_event)
{
  RC            rc            = RC::SUCCESS;
  SessionEvent *session_event = sql_event->session_event();
  SqlResult    *sql_result    = session_event->sql_result();

  // resolve 阶段需要一个已选中的数据库，因为表/字段绑定都依赖 schema 元数据。
  Db *db = session_event->session()->get_current_db();
  if (nullptr == db) {
    LOG_ERROR("cannot find current db");
    rc = RC::SCHEMA_DB_NOT_EXIST;
    sql_result->set_return_code(rc);
    sql_result->set_state_string("no db selected");
    return rc;
  }

  ParsedSqlNode *sql_node = sql_event->sql_node().get();
  Stmt          *stmt     = nullptr;

  // 把贴近 SQL 文本的 parse 结构，转换成后续执行器更容易使用的 `Stmt` 表达。
  rc = Stmt::create_stmt(db, *sql_node, stmt);
  if (rc != RC::SUCCESS && rc != RC::UNIMPLEMENTED) {
    LOG_WARN("failed to create stmt. rc=%d:%s", rc, strrc(rc));
    sql_result->set_return_code(rc);
    return rc;
  }

  // 成功后把 stmt 所有权交给事件对象，供 optimizer / executor 继续使用。
  sql_event->set_stmt(stmt);

  return rc;
}
