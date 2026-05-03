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
// Created by Wangyunlai on 2022/11/17.
//

#pragma once

#include "common/sys/rc.h"
#include "common/lang/string.h"
#include "common/lang/memory.h"

/**
 * @file communicator.h
 * @brief 定义 Observer 网络层的连接抽象与协议工厂。
 */

struct ConnectionContext;
class SessionEvent;
class Session;
class BufferedWriter;

/**
 * @defgroup Communicator
 * @brief 负责处理与客户端的通讯
 * @details 当前有两种通讯协议，一种是普通的文本协议，以'\0'作为结尾，一种是mysql协议。
 */

/**
 * @brief 负责与单个客户端连接通讯。
 * @ingroup Communicator
 *
 * @details 在listener接收到一个新的连接(参考 server.cpp::accept), 就创建一个Communicator对象。
 * 并调用 init 进行初始化。线程模型只感知这个抽象，不直接接触协议细节：
 * read_event 负责从连接读取一条完整请求并在需要时构造 SessionEvent，
 * write_result 负责把 SQL 处理结果编码回协议响应。
 * TODO 这里的 communicator 把协议和通讯方式放在了一起。plain 和 mysql 都是一种协议，
 * 他们的通讯手段都是一样的。
 */
class Communicator
{
public:
  virtual ~Communicator();

  /**
   * @brief 接收到一个新连接后初始化通讯上下文。
   * @param fd 连接对应的文件描述符。
   * @param session 与连接绑定的 Session，对象所有权转移给 communicator。
   * @param addr 对端地址字符串，仅用于日志和诊断。
   */
  virtual RC init(int fd, unique_ptr<Session> session, const string &addr);

  /**
   * @brief 读取并解析一条客户端请求。
   * @details 如果当前读到的数据只完成了握手、心跳或其他无需进入 SQL 执行链路的协议交互，
   * 可以返回 RC::SUCCESS 且让 event 保持为空；只有真正需要执行业务时才创建 SessionEvent。
   * @param[out] event 解析得到的请求事件；由调用方负责释放。
   */
  virtual RC read_event(SessionEvent *&event) = 0;

  /**
   * @brief 在任务处理完成后，通过此接口将结果返回给客户端。
   * @param event 任务数据，包括原始请求和执行结果。
   * @param[out] need_disconnect 是否需要断开连接；只有该值为 true 时，上层才会销毁连接。
   * @return 协议编码/发送结果。即使返回值不是 SUCCESS，也不能直接断开连接，仍需以
   * need_disconnect 为准判断连接是否还能继续复用。
   */
  virtual RC write_result(SessionEvent *event, bool &need_disconnect) = 0;

  /**
   * @brief 关联的会话信息
   */
  Session *session() const { return session_.get(); }

  /**
   * @brief 对端地址
   * 如果是unix socket，可能没有意义
   */
  const char *addr() const { return addr_.c_str(); }

  /**
   * @brief 关联的文件描述符
   */
  int fd() const { return fd_; }

protected:
  unique_ptr<Session> session_;        ///< 与连接绑定的会话，保存执行模式、调试开关等上下文。
  string              addr_;           ///< 对端地址，仅用于日志和诊断信息输出。
  BufferedWriter     *writer_ = nullptr;  ///< 响应缓冲写入器，统一管理 socket 写路径。
  int                 fd_     = -1;      ///< 连接 fd；派生类可将其置为 -1 以绕过默认关闭行为。
};

/**
 * @brief 当前支持的通讯协议
 * @ingroup Communicator
 */
enum class CommunicateProtocol
{
  PLAIN,  ///< 以'\0'结尾的协议
  CLI,    ///< 与客户端进行交互的协议。CLI 不应该是一种协议，只是一种通讯的方式而已
  MYSQL,  ///< mysql通讯协议。具体实现参考 MysqlCommunicator
};

/**
 * @brief 通讯协议工厂。
 * @ingroup Communicator
 */
class CommunicatorFactory
{
public:
  /**
   * @brief 根据配置创建对应协议的 communicator。
   * @param protocol 目标协议类型。
   * @return 新创建的 communicator，所有权交给调用方。
   */
  Communicator *create(CommunicateProtocol protocol);
};
