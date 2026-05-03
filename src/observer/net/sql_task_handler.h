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
// Created by Wangyunlai on 2024/01/10.
//

#pragma once

#include "common/sys/rc.h"
#include "session/session_stage.h"
#include "sql/executor/execute_stage.h"
#include "sql/optimizer/optimize_stage.h"
#include "sql/parser/parse_stage.h"
#include "sql/parser/resolve_stage.h"
#include "sql/query_cache/query_cache_stage.h"

/**
 * @file sql_task_handler.h
 * @brief 串联网络请求与 SQL 执行流水线。
 */

class Communicator;
class SQLStageEvent;

/**
 * @brief SQL 请求处理器。
 * @ingroup SQL
 * @details 对外暴露“读请求 -> 执行 SQL -> 回写响应”的一站式入口，内部依次复用 session、
 * cache、parse、resolve、optimize、execute 等阶段对象。
 */
class SqlTaskHandler
{
public:
  SqlTaskHandler()          = default;
  virtual ~SqlTaskHandler() = default;

  /**
   * @brief 处理某个连接上的一次可读事件。
   * @details 步骤包含接收请求、执行 SQL、写回应答和清理当前会话状态。
   * @param communicator 连接对象。
   * @return RC 如果返回失败，上层通常需要断开连接。
   */
  RC handle_event(Communicator *communicator);

  /**
   * @brief 执行 SQL 流水线。
   * @param sql_event 包含 SQL 文本与中间结果的阶段事件。
   * @return 任一阶段失败都会立即返回对应错误码。
   */
  RC handle_sql(SQLStageEvent *sql_event);

private:
  SessionStage    session_stage_;      ///< 绑定当前线程的 session/request 上下文。
  QueryCacheStage query_cache_stage_;  ///< 查询缓存阶段。
  ParseStage      parse_stage_;        ///< 解析阶段，将 SQL 文本转换为语法树。
  ResolveStage    resolve_stage_;      ///< 语义解析阶段，将语法树转换为 Stmt。
  OptimizeStage   optimize_stage_;     ///< 优化阶段，生成更适合执行的计划。
  ExecuteStage    execute_stage_;      ///< 执行阶段，真正产出 SqlResult。
};
