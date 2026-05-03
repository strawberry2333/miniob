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

#include "session_stage.h"

#include <string.h>

#include "common/conf/ini.h"
#include "common/lang/mutex.h"
#include "common/lang/string.h"
#include "common/log/log.h"
#include "event/session_event.h"
#include "event/sql_event.h"
#include "net/communicator.h"
#include "net/server.h"
#include "session/session.h"

using namespace common;

/**
 * @brief SessionStage 的实现文件。
 * @details 这里负责建立会话上下文、串行调用 SQL 各处理阶段，并在末尾统一回写结果。
 */

/// @brief 默认析构，不包含额外资源释放逻辑。
SessionStage::~SessionStage() {}

/**
 * @brief 以同步方式处理一个完整的客户端请求。
 * @param sev 当前请求事件。
 */
void SessionStage::handle_request(SessionEvent *sev)
{
  string sql = sev->query();
  if (common::is_blank(sql.c_str())) {
    // 空白语句无需进入 SQL 处理流水线。
    return;
  }

  // 先把当前会话与请求绑定到线程上下文，方便后续阶段按需读取。
  Session::set_current_session(sev->session());
  sev->session()->set_current_request(sev);
  SQLStageEvent sql_event(sev, sql);
  (void)handle_sql(&sql_event);

  // SQL 处理完成后统一交给 communicator 选择协议格式并回写结果。
  Communicator *communicator    = sev->get_communicator();
  bool          need_disconnect = false;
  RC            rc              = communicator->write_result(sev, need_disconnect);
  LOG_INFO("write result return %s", strrc(rc));
  if (need_disconnect) {
    // do nothing
  }
  sev->session()->set_current_request(nullptr);
  Session::set_current_session(nullptr);
}

/**
 * @brief 预留的请求入口，只完成基础上下文准备。
 * @param event 当前请求事件。
 */
void SessionStage::handle_request2(SessionEvent *event)
{
  const string &sql = event->query();
  if (common::is_blank(sql.c_str())) {
    return;
  }

  Session::set_current_session(event->session());
  event->session()->set_current_request(event);
  // 统一构造 SQLStageEvent，为将来切换异步调度保留统一入口。
  SQLStageEvent sql_event(event, sql);
}

/**
 * 处理一个SQL语句经历这几个阶段。
 * 虽然看起来流程比较多，但是对于大多数SQL来说，更多的可以关注parse和executor阶段。
 * 通常只有select、delete等带有查询条件的语句才需要进入optimize。
 * 对于DDL语句，比如create table、create index等，没有对应的查询计划，可以直接搜索
 * create_table_executor、create_index_executor来看具体的执行代码。
 * select、delete等DML语句，会产生一些执行计划，如果感觉繁琐，可以跳过optimize直接看
 * execute_stage中的执行，通过explain语句看需要哪些operator，然后找对应的operator来
 * 调试或者看代码执行过程即可。
 */
RC SessionStage::handle_sql(SQLStageEvent *sql_event)
{
  // 先尝试命中查询缓存，命中失败或无需缓存时再进入真正的 SQL 流水线。
  RC rc = query_cache_stage_.handle_request(sql_event);
  if (OB_FAIL(rc)) {
    LOG_TRACE("failed to do query cache. rc=%s", strrc(rc));
    return rc;
  }

  // parse 阶段把 SQL 文本翻译成语法树。
  rc = parse_stage_.handle_request(sql_event);
  if (OB_FAIL(rc)) {
    LOG_TRACE("failed to do parse. rc=%s", strrc(rc));
    return rc;
  }

  // resolve 阶段把语法树绑定到真实 schema，生成可执行的 Stmt。
  rc = resolve_stage_.handle_request(sql_event);
  if (OB_FAIL(rc)) {
    LOG_TRACE("failed to do resolve. rc=%s", strrc(rc));
    return rc;
  }

  // optimize 阶段允许返回 UNIMPLEMENTED，表示当前语句无需专门优化。
  rc = optimize_stage_.handle_request(sql_event);
  if (rc != RC::UNIMPLEMENTED && rc != RC::SUCCESS) {
    LOG_TRACE("failed to do optimize. rc=%s", strrc(rc));
    return rc;
  }

  // execute 阶段负责真正执行物理计划，并把结果写入 SessionEvent::sql_result()。
  rc = execute_stage_.handle_request(sql_event);
  if (OB_FAIL(rc)) {
    LOG_TRACE("failed to do execute. rc=%s", strrc(rc));
    return rc;
  }

  return rc;
}
