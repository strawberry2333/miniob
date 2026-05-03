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

#pragma once

#include "sql/executor/execute_stage.h"
#include "sql/optimizer/optimize_stage.h"
#include "sql/parser/parse_stage.h"
#include "sql/parser/resolve_stage.h"
#include "sql/query_cache/query_cache_stage.h"

/**
 * @brief SQL 请求入口 stage 定义。
 * @details SessionStage 是 observer SQL 处理链的第一个逻辑阶段，负责建立线程上下文并串联后续各 stage。
 */

/**
 * @brief SEDA处理的stage
 * @defgroup SQLStage
 * @details 收到的客户端请求会放在SEDA框架中处理，每个stage都是一个处理阶段。
 * 当前的处理流程可以通过observer.ini配置文件查看。
 * seda::stage使用说明：
 * 这里利用seda的线程池与调度。stage是一个事件处理的几个阶段。
 * 目前包括session,parse,execution和storage
 * 每个stage使用handleEvent函数处理任务，并且使用StageEvent::pushCallback注册回调函数。
 * 这时当调用StageEvent::done(Immediate)时，就会调用该事件注册的回调函数，如果没有回调函数，就会释放自己。
 */

/**
 * @brief SQL处理的session阶段，也是第一个阶段
 * @ingroup SQLStage
 * @details 这个阶段离网络入口最近，负责把 SessionEvent 转成 SQLStageEvent 并驱动 SQL 处理流水线。
 */
class SessionStage
{
public:
  SessionStage() = default;
  /// @brief 默认析构；各子 stage 成员会自动销毁。
  virtual ~SessionStage();

public:
  /**
   * @brief 预留的另一条请求入口。
   * @param event 当前网络请求事件。
   * @note 目前只完成会话绑定和 SQLStageEvent 构造，没有继续推进完整流水线。
   */
  void handle_request2(SessionEvent *event);

public:
  /**
   * @brief 同步处理一条客户端请求并把结果写回客户端。
   * @param event 当前网络请求事件。
   */
  void handle_request(SessionEvent *event);
  /**
   * @brief 驱动单条 SQL 依次经过各个处理阶段。
   * @param sql_event 当前 SQL 的阶段事件。
   * @return RC 执行状态码；一旦某个阶段失败就立刻返回。
   */
  RC   handle_sql(SQLStageEvent *sql_event);

private:
  QueryCacheStage query_cache_stage_;
  ParseStage      parse_stage_;
  ResolveStage    resolve_stage_;
  OptimizeStage   optimize_stage_;
  ExecuteStage    execute_stage_;
};
