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
// Created by Wangyunlai on 2024/01/11.
//

#pragma once

#include "net/thread_handler.h"
#include "net/sql_task_handler.h"
#include "common/thread/thread_pool_executor.h"
#include "common/lang/mutex.h"

/**
 * @file java_thread_pool_thread_handler.h
 * @brief 基于 libevent + 线程池 的连接处理模型。
 */

struct EventCallbackAg;

/**
 * @brief 基于事件循环和工作线程池的连接处理模型。
 * @ingroup ThreadHandler
 * @details 一个专门线程运行 libevent 主循环，负责探测连接上的可读事件；真正的 SQL 解析、
 * 执行和回包放入线程池异步处理。单个连接在任意时刻最多只会有一个任务在执行，依靠“事件处理前
 * 暂不重新注册读事件”的策略避免同一连接并发读写。
 * libevent 是一个常用并且高效的异步事件消息库，可以阅读手册了解更多。
 * [libevent 手册](https://libevent.org/doc/index.html)
 */
class JavaThreadPoolThreadHandler : public ThreadHandler
{
public:
  JavaThreadPoolThreadHandler() = default;
  virtual ~JavaThreadPoolThreadHandler();

  //! @copydoc ThreadHandler::start
  virtual RC start() override;
  //! @copydoc ThreadHandler::stop
  virtual RC stop() override;
  //! @copydoc ThreadHandler::await_stop
  virtual RC await_stop() override;

  //! @copydoc ThreadHandler::new_connection
  virtual RC new_connection(Communicator *communicator) override;
  //! @copydoc ThreadHandler::close_connection
  virtual RC close_connection(Communicator *communicator) override;

public:
  /**
   * @brief 处理一次 libevent 交付的连接事件。
   * @details 该函数本身运行在事件循环线程中，只做轻量分发，把真正耗时的 SQL 任务提交到线程池。
   * @param ag 事件回调参数，包含 event 对象、连接对象以及回调宿主。
   */
  void handle_event(EventCallbackAg *ag);

  /**
   * @brief libevent 事件循环主体。
   * @details 该函数会长期阻塞在 event_base 上，占据线程池中的一个工作线程直到 stop。
   */
  void event_loop_thread();

private:
  mutex                                  lock_;  ///< 保护 event_map_ 的并发访问。
  struct event_base                     *event_base_ = nullptr;  ///< libevent 的事件基座。
  common::ThreadPoolExecutor             executor_;  ///< 同时承载事件循环线程和 SQL 工作线程。
  map<Communicator *, EventCallbackAg *> event_map_;  ///< 连接到 libevent 回调上下文的映射。

  SqlTaskHandler sql_task_handler_;  ///< SQL 请求处理器。
};
