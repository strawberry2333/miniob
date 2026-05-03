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

#include "session_event.h"
#include "net/communicator.h"

/**
 * @brief SessionEvent 的实现文件。
 * @details 这里负责把网络层的 Communicator 与 SQL 层需要的会话/结果对象接起来。
 */

/**
 * @brief 初始化一次请求的顶层上下文。
 * @param comm 与客户端连接关联的通信对象。
 */
SessionEvent::SessionEvent(Communicator *comm) : communicator_(comm), sql_result_(communicator_->session()) {}

/// @brief 默认析构；成员对象会随 SessionEvent 自动销毁。
SessionEvent::~SessionEvent() {}

/// @brief 返回当前请求所依附的通信对象。
Communicator *SessionEvent::get_communicator() const { return communicator_; }

/// @brief 通过 communicator 反查会话，避免事件内维护重复状态。
Session *SessionEvent::session() const { return communicator_->session(); }
