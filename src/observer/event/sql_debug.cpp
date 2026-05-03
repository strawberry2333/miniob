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
// Created by Wangyunlai on 2023/6/29.
//

#include <stdarg.h>

#include "event/session_event.h"
#include "event/sql_debug.h"
#include "session/session.h"

/**
 * @brief SQL 调试信息实现。
 * @details 调试文本与 SessionEvent 同生命周期，执行阶段写入，输出阶段统一带回客户端。
 */

/// @brief 向当前请求追加一条调试文本。
void SqlDebug::add_debug_info(const string &debug_info) { debug_infos_.push_back(debug_info); }

/// @brief 清空当前请求的调试文本集合。
void SqlDebug::clear_debug_info() { debug_infos_.clear(); }

/// @brief 以只读方式暴露内部调试文本列表，避免额外拷贝。
const list<string> &SqlDebug::get_debug_infos() const { return debug_infos_; }

/**
 * @brief 向当前 SQL 请求附加一条格式化调试信息。
 * @param fmt printf 风格格式串。
 */
void sql_debug(const char *fmt, ...)
{
  Session *session = Session::current_session();
  if (nullptr == session) {
    // 只有在 SQL 请求线程里才存在可挂载调试信息的上下文。
    return;
  }

  SessionEvent *request = session->current_request();
  if (nullptr == request) {
    // 会话存在但当前没有活动请求时，不生成调试信息。
    return;
  }

  SqlDebug &sql_debug = request->sql_debug();

  const int buffer_size = 4096;
  char     *str         = new char[buffer_size];

  va_list ap;
  va_start(ap, fmt);
  // 先格式化成普通字符串，再同时写入请求上下文和服务端日志。
  vsnprintf(str, buffer_size, fmt, ap);
  va_end(ap);

  sql_debug.add_debug_info(str);
  LOG_DEBUG("sql debug info: [%s]", str);

  delete[] str;
}
