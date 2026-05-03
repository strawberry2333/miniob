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

#include "net/plain_communicator.h"

/**
 * @file cli_communicator.h
 * @brief 面向命令行交互模式的通讯器。
 */

/**
 * @brief 用于命令行模式的通讯器。
 * @ingroup Communicator
 * @details 直接通过终端/标准输入输出进行通讯。从这里的实现来看，是不需要libevent的一些实现的，
 * 因此communicator需要重构，或者需要重构server中的各个通讯启动模式。
 */
class CliCommunicator : public PlainCommunicator
{
public:
  CliCommunicator() = default;
  virtual ~CliCommunicator();

  /**
   * @brief 初始化 CLI 模式的读写端。
   * @details 当前只支持 `fd == STDIN_FILENO`，输入来自标准输入，输出改写到标准输出。
   */
  RC init(int fd, unique_ptr<Session> session, const string &addr) override;

  /**
   * @brief 从 readline 读取一行命令并包装为 SessionEvent。
   * @details 空行、空白输入和 exit 命令都不会生成事件。
   */
  RC read_event(SessionEvent *&event) override;

  /**
   * @brief 复用 PlainCommunicator 的文本输出逻辑。
   * @details CLI 模式下单次写失败不意味着应立刻断开“连接”，因此会屏蔽 need_disconnect。
   */
  RC write_result(SessionEvent *event, bool &need_disconnect) override;

  bool exit() const { return exit_; }

private:
  bool exit_ = false;  ///< 是否收到了用户的退出命令。

  int write_fd_ = -1;  ///< CLI 模式的输出 fd，通常是标准输出。
};
