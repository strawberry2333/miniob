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
// Created by Wangyunlai on 2023/4/25.
//

#pragma once

#include "common/sys/rc.h"

class SQLStageEvent;

/**
 * @file command_executor.h
 * @brief 定义直接执行命令类语句的统一入口。
 */

/**
 * @defgroup Executor
 * @brief 一些SQL语句不会生成对应的执行计划，直接使用Executor来执行，比如DDL语句
 */

/**
 * @brief 命令语句分发器。
 * @ingroup Executor
 * @details 这类语句通常不会下沉为物理算子树，而是直接调用对应 executor
 * 完成 schema 变更、会话变量修改或事务控制等动作。
 */
class CommandExecutor
{
public:
  CommandExecutor()          = default;
  virtual ~CommandExecutor() = default;

  /**
   * @brief 根据 `StmtType` 分派到具体命令执行器。
   * @param sql_event 当前 SQL 请求上下文。
   * @return 返回具体命令执行状态。
   */
  RC execute(SQLStageEvent *sql_event);
};
