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

#include "common/sys/rc.h"

class SQLStageEvent;
class SessionEvent;
class SelectStmt;

/**
 * @file execute_stage.h
 * @brief 定义 SQL 流水线中的执行阶段。
 */

/**
 * @brief 执行SQL语句的Stage，包括DML和DDL
 * @ingroup SQLStage
 * @details 根据前面阶段生成的结果，有些语句会生成执行计划，有些不会。
 * 整体上分为两类，带执行计划的，或者 CommandExecutor 可以直接执行的。
 */
class ExecuteStage
{
public:
  /**
   * @brief 执行当前 SQL 请求。
   * @param event 当前 SQL 请求上下文。
   * @return 返回执行阶段状态；若生成物理算子则把算子挂到 `SqlResult` 上。
   */
  RC handle_request(SQLStageEvent *event);

  /**
   * @brief 接管已经生成好的物理算子树。
   * @param sql_event 当前 SQL 请求上下文。
   * @return 返回算子转移结果。
   */
  RC handle_request_with_physical_operator(SQLStageEvent *sql_event);
};
