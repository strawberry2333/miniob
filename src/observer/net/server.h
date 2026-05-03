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
// Created by Longda on 2021/4/1.
//

#pragma once

#include "net/server_param.h"

/**
 * @file server.h
 * @brief 定义 Observer 的网络入口，包括 TCP/Unix Socket 服务端和命令行模式服务端。
 */

class Communicator;
class ThreadHandler;

/**
 * @brief 负责接收客户端连接并驱动后续请求处理。
 * @ingroup Communicator
 * @details 当前支持网络连接，有TCP和Unix Socket两种方式。通过命令行参数来指定使用哪种方式。
 * 启动后监听端口或 unix socket，当有新的连接到达时，创建一个 Communicator 对象并交给
 * ThreadHandler 管理。
 */
class Server
{
public:
  Server(const ServerParam &input_server_param) : server_param_(input_server_param) {}
  virtual ~Server() {}

  /**
   * @brief 进入服务主循环。
   * @return 0 表示正常退出，其它值表示启动或运行阶段发生错误。
   */
  virtual int  serve()    = 0;

  /**
   * @brief 请求服务停止。
   * @details 一般只修改停止标记，实际资源回收由 serve 退出路径统一处理。
   */
  virtual void shutdown() = 0;

protected:
  ServerParam server_param_;  ///< 服务启动参数
};

class NetServer : public Server
{
public:
  NetServer(const ServerParam &input_server_param);
  virtual ~NetServer();

public:
  int  serve() override;
  void shutdown() override;

private:
  /**
   * @brief 接收到新的连接时，创建并初始化 Communicator。
   * @details 该函数在监听循环中被同步调用，负责完成 accept、socket 参数配置、session
   * 创建以及把连接移交给线程模型。
   * @param fd 监听 socket 的描述符。
   */
  void accept(int fd);

private:
  /**
   * @brief 将socket描述符设置为非阻塞模式
   *
   * @param fd 指定的描述符。
   * @return 0 表示成功，-1 表示系统调用失败。
   */
  int set_non_block(int fd);

  /**
   * @brief 根据启动参数选择具体监听方式。
   * @return 0 表示成功，-1 表示失败。
   */
  int start();

  /**
   * @brief 启动TCP服务
   * @return 0 表示成功，-1 表示失败。
   */
  int start_tcp_server();

  /**
   * @brief 启动Unix Socket服务
   * @return 0 表示成功，-1 表示失败。
   */
  int start_unix_socket_server();

private:
  volatile bool started_ = false;  ///< 监听循环是否继续运行。

  int server_socket_ = -1;  ///< 监听套接字描述符。

  CommunicatorFactory communicator_factory_;  ///< 根据协议类型创建连接通讯器。
  ThreadHandler      *thread_handler_ = nullptr;  ///< 管理连接生命周期与执行线程模型。
};

/**
 * @brief 直接使用标准输入输出运行的服务端。
 * @details 主要用于命令行模式和调试，不涉及监听 socket 或多连接管理。
 */
class CliServer : public Server
{
public:
  CliServer(const ServerParam &input_server_param);
  virtual ~CliServer();

  int  serve() override;
  void shutdown() override;

private:
  volatile bool started_ = false;  ///< CLI 读写循环是否继续运行。
};
