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
// Created by Longda on 2021/4/14.
//

#include "event/sql_event.h"

#include "event/session_event.h"
#include "sql/stmt/stmt.h"

/**
 * @brief SQLStageEvent 的实现文件。
 * @details 它负责在 SQL 处理各阶段之间转移中间结果，并在生命周期结束时清理手动管理的对象。
 */

/**
 * @brief 初始化一条 SQL 的阶段事件。
 * @param event 顶层请求事件。
 * @param sql 当前待处理的 SQL 文本。
 */
SQLStageEvent::SQLStageEvent(SessionEvent *event, const string &sql) : session_event_(event), sql_(sql) {}

/**
 * @brief 清理阶段事件内持有的中间对象。
 * @details session_event_ 的所有权属于外层请求，这里只断开引用；stmt_ 需要显式释放。
 */
SQLStageEvent::~SQLStageEvent() noexcept
{
  if (session_event_ != nullptr) {
    // 外层 SessionEvent 由请求生命周期托管，这里只清空借用指针。
    session_event_ = nullptr;
  }

  if (stmt_ != nullptr) {
    // resolver 返回的是堆对象，事件结束时统一回收。
    delete stmt_;
    stmt_ = nullptr;
  }
}
