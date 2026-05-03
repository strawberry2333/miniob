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

#include "common/lang/string.h"
#include "event/sql_debug.h"
#include "sql/executor/sql_result.h"

class Session;
class Communicator;

/**
 * @brief SQL 顶层请求事件定义。
 * @details SessionEvent 把网络连接、原始 SQL、执行结果和调试输出打包在一起，
 * 供 session stage 以及后续 SQL 处理阶段共享。
 */
class SessionEvent
{
public:
  /**
   * @brief 构造一次客户端请求的上下文对象。
   * @param client 与当前连接关联的通信对象，不能为空。
   * @note sql_result_ 需要立刻绑定 session，因此 communicator_ 必须已关联有效 Session。
   */
  SessionEvent(Communicator *client);

  /// @brief 析构请求事件，不负责释放 communicator_。
  virtual ~SessionEvent();

  /// @brief 返回当前请求对应的通信对象。
  Communicator *get_communicator() const;

  /// @brief 返回当前请求所属的会话对象。
  Session      *session() const;

  /// @brief 设置原始 SQL 文本。
  void set_query(const string &query) { query_ = query; }

  /// @brief 返回原始 SQL 文本。
  const string &query() const { return query_; }
  /// @brief 返回可写的 SQL 结果对象，供各阶段填充执行结果。
  SqlResult    *sql_result() { return &sql_result_; }
  /// @brief 返回调试信息收集器，供执行阶段追加 debug 文本。
  SqlDebug     &sql_debug() { return sql_debug_; }

private:
  Communicator *communicator_ = nullptr;  ///< 与客户端通讯的对象，由网络层管理生命周期
  SqlResult     sql_result_;              ///< SQL 执行结果，最终由 communicator 回写给客户端
  SqlDebug      sql_debug_;               ///< 当前 SQL 语句累计的调试输出
  string        query_;                   ///< 原始 SQL 文本
};
