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

#include "common/sys/rc.h"
#include "event/session_event.h"
#include "event/sql_event.h"
#include "session/session.h"
#include "sql/executor/sql_result.h"
#include "sql/operator/string_list_physical_operator.h"
#include "storage/db/db.h"

/**
 * @file show_tables_executor.h
 * @brief 定义 `SHOW TABLES` 命令执行器。
 */

/**
 * @brief 枚举当前数据库中所有表名的命令执行器。
 * @ingroup Executor
 * @note 与CreateIndex类似，不处理并发
 */
class ShowTablesExecutor
{
public:
  ShowTablesExecutor()          = default;
  virtual ~ShowTablesExecutor() = default;

  /**
   * @brief 执行 `SHOW TABLES`。
   * @param sql_event 当前 SQL 请求上下文。
   * @return 总是返回 `RC::SUCCESS`。
   */
  RC execute(SQLStageEvent *sql_event)
  {
    SqlResult    *sql_result    = sql_event->session_event()->sql_result();
    SessionEvent *session_event = sql_event->session_event();

    // 直接从当前数据库拉取表名列表，再包装成单列表结果集。
    Db *db = session_event->session()->get_current_db();

    vector<string> all_tables;
    db->all_tables(all_tables);

    TupleSchema tuple_schema;
    tuple_schema.append_cell(TupleCellSpec("", "Tables_in_SYS", "Tables_in_SYS"));
    sql_result->set_tuple_schema(tuple_schema);

    auto oper = new StringListPhysicalOperator;
    // 每个表名输出成结果集中的一行。
    for (const string &s : all_tables) {
      oper->append(s);
    }

    sql_result->set_operator(unique_ptr<PhysicalOperator>(oper));
    return RC::SUCCESS;
  }
};
