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
#include "storage/trx/trx.h"

/**
 * @file trx_begin_executor.h
 * @brief 定义 `BEGIN` 命令执行器。
 */

/**
 * @brief 打开多语句事务模式的命令执行器。
 * @ingroup Executor
 */
class TrxBeginExecutor
{
public:
  TrxBeginExecutor()          = default;
  virtual ~TrxBeginExecutor() = default;

  /**
   * @brief 执行 `BEGIN`。
   * @param sql_event 当前 SQL 请求上下文。
   * @return 返回事务启动结果。
   */
  RC execute(SQLStageEvent *sql_event)
  {
    SessionEvent *session_event = sql_event->session_event();

    Session *session = session_event->session();
    Trx     *trx     = session->current_trx();

    // 打开多语句事务模式后，后续语句的提交/回滚交由显式 COMMIT/ROLLBACK 控制。
    session->set_trx_multi_operation_mode(true);

    return trx->start_if_need();
  }
};
