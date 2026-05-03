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

#include "net/communicator.h"
#include "common/lang/string.h"

/**
 * @file server_param.h
 * @brief 服务端启动参数定义。
 */

/**
 * @brief 服务端启动参数
 * @ingroup Communicator
 * @details 统一收敛网络入口、协议选择和线程模型配置，供 NetServer/CliServer 在启动阶段读取。
 */
class ServerParam
{
public:
  /**
   * @brief 使用默认监听地址、端口和连接数初始化参数。
   */
  ServerParam();

  ServerParam(const ServerParam &other) = default;
  ~ServerParam()                        = default;

public:
  long listen_addr;  ///< 监听地址，默认 `INADDR_ANY`，表示接受任意来源的连接。

  int max_connection_num;  ///< 监听 socket 的 backlog 上限。

  int port;  ///< TCP 监听端口号。

  string unix_socket_path;  ///< Unix domain socket 文件路径。

  bool use_std_io = false;  ///< 是否使用标准输入输出而不是网络 socket。

  bool use_unix_socket = false;  ///< 是否使用 Unix domain socket 监听。

  CommunicateProtocol protocol;  ///< 通讯协议，目前支持文本协议、CLI 和 MySQL 协议。

  string thread_handling;  ///< 线程模型名，例如 `one-thread-per-connection`。
};
