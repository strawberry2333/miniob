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

#pragma once

#include "common/seda/stage_event.h"

class SQLStageEvent;

/**
 * @brief storage stage 使用的事件包装器。
 * @details 某些 SEDA stage 只接受 StageEvent 接口，因此这里用一个轻量外壳把 SQLStageEvent 包起来。
 */
class StorageEvent : public common::StageEvent
{
public:
  /// @brief 使用现有 SQLStageEvent 构造一个 storage 事件，不转移所有权。
  StorageEvent(SQLStageEvent *sql_event) : sql_event_(sql_event) {}

  /// @brief 默认析构，具体释放策略由实现方决定。
  virtual ~StorageEvent();

  /// @brief 返回被包装的 SQLStageEvent，供 storage stage 继续处理。
  SQLStageEvent *sql_event() const { return sql_event_; }

private:
  SQLStageEvent *sql_event_;  ///< 指向上层 SQL 处理事件；StorageEvent 本身不拥有它。
};
