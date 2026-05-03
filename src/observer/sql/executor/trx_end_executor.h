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
#include "sql/stmt/stmt.h"
#include "storage/trx/trx.h"

/**
 * @file trx_end_executor.h
 * @brief 定义 `COMMIT` / `ROLLBACK` 命令执行器。
 */

/**
 * @brief 结束显式事务的命令执行器，可以是提交或回滚。
 * @ingroup Executor
 */
class TrxEndExecutor
{
public:
  TrxEndExecutor()          = default;
  virtual ~TrxEndExecutor() = default;

  /**
   * @brief 执行 `COMMIT` 或 `ROLLBACK`。
   * @param sql_event 当前 SQL 请求上下文。
   * @return 返回事务收尾结果。
   */
  RC execute(SQLStageEvent *sql_event)
  {
    RC            rc            = RC::SUCCESS;
    Stmt         *stmt          = sql_event->stmt();
    SessionEvent *session_event = sql_event->session_event();

    Session *session = session_event->session();
    // 一旦显式结束事务，就恢复单语句自动事务行为。
    session->set_trx_multi_operation_mode(false);
    Trx *trx = session->current_trx();

    // 语句类型已经在上游绑定成 COMMIT 或 ROLLBACK，这里只做一次显式分支。
    if (stmt->type() == StmtType::COMMIT) {
      rc = trx->commit();
    } else {
      rc = trx->rollback();
    }
    // 事务对象无论提交还是回滚都需要销毁，下一条语句再按需新建。
    session->destroy_trx();
    return rc;
  }
};
