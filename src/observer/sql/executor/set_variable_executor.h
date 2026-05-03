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
// Created by Wangyunlai on 2023/6/14.
//

#pragma once

#include "common/sys/rc.h"
#include "event/session_event.h"
#include "event/sql_event.h"
#include "session/session.h"
#include "sql/stmt/set_variable_stmt.h"

/**
 * @file set_variable_executor.h
 * @brief 定义 `SET` 会话变量命令执行器。
 */

/**
 * @brief 会话变量设置命令执行器。
 * @ingroup Executor
 */
class SetVariableExecutor
{
public:
  SetVariableExecutor()          = default;
  virtual ~SetVariableExecutor() = default;

  /**
   * @brief 执行 `SET variable = value`。
   * @param sql_event 当前 SQL 请求上下文。
   * @return 返回变量解析与写入结果。
   */
  RC execute(SQLStageEvent *sql_event);

private:
  /**
   * @brief 把通用 `Value` 转换成布尔配置值。
   * @param var_value 输入 SQL 值。
   * @param bool_value 输出布尔结果。
   * @return 转换成功返回 `RC::SUCCESS`。
   */
  RC var_value_to_boolean(const Value &var_value, bool &bool_value) const;

  /**
   * @brief 把字符串配置值转换为执行模式枚举。
   * @param var_value 输入 SQL 值。
   * @param execution_mode 输出执行模式。
   * @return 转换成功返回 `RC::SUCCESS`。
   */
  RC get_execution_mode(const Value &var_value, ExecutionMode &execution_mode) const;
};
