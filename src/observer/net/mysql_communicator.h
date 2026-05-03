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
// Created by Wangyunlai on 2022/11/22.
//

#pragma once

#include "net/communicator.h"
#include "common/lang/string.h"

/**
 * @file mysql_communicator.h
 * @brief MySQL 文本协议通讯器。
 */

class SqlResult;
class BasePacket;

/**
 * @brief 与 MySQL 客户端通讯。
 * @ingroup Communicator
 * @ingroup MySQLProtol
 * @details 实现连接握手、命令读取、结果集编码和状态回包等 MySQL 文本协议流程。
 * 对外仍暴露 Communicator 抽象，对内维护认证状态、客户端 capability 以及当前请求的
 * sequence id。
 * 可以参考 [MySQL Page Protocol](https://dev.mysql.com/doc/dev/mysql-server/latest/PAGE_PROTOCOL.html)
 * 或 [MariaDB Protocol](https://mariadb.com/kb/en/clientserver-protocol/)
 */
class MysqlCommunicator : public Communicator
{
public:
  /**
   * @brief 连接刚开始建立时，进行一些初始化
   * @details 参考MySQL或MariaDB的手册，服务端要首先向客户端发送一个握手包，等客户端回复后，
   * 再回复一个OkPacket或ErrPacket
   */
  virtual RC init(int fd, unique_ptr<Session> session, const string &addr) override;

  /**
   * @brief 有新的消息到达时，接收消息
   * @details 因为MySQL协议的特殊性，收到数据后不一定需要向后流转，比如握手包
   */
  virtual RC read_event(SessionEvent *&event) override;

  /**
   * @brief 将处理结果返回给客户端
   * @param[in] event 任务数据，包括处理的结果
   * @param[out] need_disconnect 是否需要断开连接
   */
  virtual RC write_result(SessionEvent *event, bool &need_disconnect) override;

private:
  /**
   * @brief 发送一个已编码的协议包到客户端。
   *
   * @param[in] packet 要发送的数据包。
   */
  RC send_packet(const BasePacket &packet);

  /**
   * @brief 返回执行状态，而不是结果集行数据。
   * @details 适用于 DDL/DML 或执行失败等无需返回表格数据的场景。
   *
   * @param[in] event 处理的结果。
   * @param[out] need_disconnect 是否需要断开连接。
   */
  RC write_state(SessionEvent *event, bool &need_disconnect);

  /**
   * @brief 返回客户端列描述信息
   * @details 根据MySQL text protocol 描述，普通的结果分为列信息描述和行数据。
   * 这里就分为两个函数
   */
  RC send_column_definition(SqlResult *sql_result, bool &need_disconnect);

  /**
   * @brief 返回客户端行数据
   *
   * @param[in] event 任务数据
   * @param[in] sql_result 返回的结果
   * @param no_column_def 是否没有列描述信息
   * @param[out] need_disconnect 是否需要断开连接
   * @return RC
   */
  RC send_result_rows(SessionEvent *event, SqlResult *sql_result, bool no_column_def, bool &need_disconnect);

  /**
   * @brief 根据实际测试，客户端在连接上来时，会发起一个 version_comment的查询
   * @details 这里就针对这个查询返回一个结果
   */
  RC handle_version_comment(bool &need_disconnect);

  /**
   * @brief 逐 tuple 输出 MySQL 文本结果集行包。
   * @param sql_result 已经 open 的结果集。
   * @param packet 复用的行缓冲区。
   * @param[out] affected_rows 已经发送的行数。
   * @param[out] need_disconnect 写失败时置为 true。
   */
  RC write_tuple_result(SqlResult *sql_result, vector<char> &packet, int &affected_rows, bool &need_disconnect);

  /**
   * @brief 逐 chunk 输出 MySQL 文本结果集行包。
   * @param sql_result 已经 open 的结果集。
   * @param packet 复用的行缓冲区。
   * @param[out] affected_rows 已经发送的行数。
   * @param[out] need_disconnect 写失败时置为 true。
   */
  RC write_chunk_result(SqlResult *sql_result, vector<char> &packet, int &affected_rows, bool &need_disconnect);

private:
  bool authed_ = false;  ///< 是否已经完成握手阶段，决定 read_event 的状态机分支。

  uint32_t client_capabilities_flag_ = 0;  ///< 客户端声明的 capability，影响后续编解码路径。

  int8_t sequence_id_ = 0;  ///< 当前请求内的包序号，需按协议严格递增。
};
