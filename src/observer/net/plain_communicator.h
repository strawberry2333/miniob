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
// Created by Wangyunlai on 2023/06/25.
//

#pragma once

#include "net/communicator.h"
#include "common/lang/vector.h"

/**
 * @file plain_communicator.h
 * @brief 简单文本协议通讯器。
 */

class SqlResult;

/**
 * @brief 基于 `'\0'` 结尾报文的文本协议通讯器。
 * @ingroup Communicator
 * @details 请求以 `'\0'` 作为结束标记；响应以文本形式返回，结果集按列分隔、按行换行，
 * 整个响应再追加一个协议分隔符。
 */
class PlainCommunicator : public Communicator
{
public:
  PlainCommunicator();
  virtual ~PlainCommunicator() = default;

  RC read_event(SessionEvent *&event) override;
  RC write_result(SessionEvent *event, bool &need_disconnect) override;

private:
  /**
   * @brief 写入无结果集语句的执行状态。
   * @param event 当前请求事件。
   * @param[out] need_disconnect 写失败时置为 true。
   */
  RC write_state(SessionEvent *event, bool &need_disconnect);

  /**
   * @brief 在 SQL debug 开关开启时回写调试信息。
   * @param event 当前请求事件。
   * @param[out] need_disconnect 写失败时置为 true。
   */
  RC write_debug(SessionEvent *event, bool &need_disconnect);

  /**
   * @brief 根据 SqlResult 的形态选择具体结果编码路径。
   * @details 可能输出状态文本、元组结果或 chunk 结果。
   */
  RC write_result_internal(SessionEvent *event, bool &need_disconnect);

  /**
   * @brief 逐行拉取 Tuple 结果并编码为文本。
   * @param sql_result 已经 open 的结果集。
   */
  RC write_tuple_result(SqlResult *sql_result);

  /**
   * @brief 逐 chunk 拉取结果并编码为文本。
   * @param sql_result 已经 open 的结果集。
   */
  RC write_chunk_result(SqlResult *sql_result);

protected:
  vector<char> send_message_delimiter_;  ///< 每条响应末尾追加的协议分隔符。
  vector<char> debug_message_prefix_;    ///< 调试信息前缀，避免与结果正文混淆。
};
