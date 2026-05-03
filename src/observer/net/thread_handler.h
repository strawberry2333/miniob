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

/**
 * @file thread_handler.h
 * @brief 定义连接处理线程模型的统一抽象。
 */

class Communicator;

/**
 * @defgroup  ThreadHandler
 * @brief 线程模型接口。
 * @details 处理连接上所有的消息。可以使用不同的模型来处理，当前有一个连接一个线程的模式和线程池模式。
 * 线程模型仅处理与客户端通讯的连接，不处理observer监听套接字。
 */
class ThreadHandler
{
public:
  ThreadHandler()          = default;
  virtual ~ThreadHandler() = default;

  /**
   * @brief 启动线程模型的内部资源。
   * @details 例如创建事件循环、工作线程或线程池；成功返回后才允许接收连接。
   */
  virtual RC start() = 0;

  /**
   * @brief 发起停止流程。
   * @details 该接口通常只负责发出停止信号，不保证所有线程已经退出。
   */
  virtual RC stop() = 0;

  /**
   * @brief 等待线程模型彻底停止。
   * @details 调用完成后，不应再持有任何活动连接或后台线程。
   */
  virtual RC await_stop() = 0;

  /**
   * @brief 注册一个新连接到线程模型中。
   * @param communicator 与客户端通讯的对象，成功后所有权由线程模型接管。
   */
  virtual RC new_connection(Communicator *communicator) = 0;

  /**
   * @brief 关闭并清理一个连接。
   * @details 通常由线程模型内部在检测到断连、发送失败或收到停止信号后调用。
   * @param communicator 与客户端通讯的对象。
   */
  virtual RC close_connection(Communicator *communicator) = 0;

public:
  /**
   * @brief 创建一个线程模型实例。
   * @param name 配置中的线程模型名字；当前支持 `one-thread-per-connection`
   * 和 `java-thread-pool`。
   * @return 新创建的线程模型；调用方负责释放。
   */
  static ThreadHandler *create(const char *name);
};
