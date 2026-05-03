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
// Created by Wangyunlai on 2023/6/13.
//

#pragma once

#include "common/sys/rc.h"

class SQLStageEvent;

/**
 * @file create_table_executor.h
 * @brief 定义 `CREATE TABLE` 命令执行器。
 */

/**
 * @brief 创建表的命令执行器。
 * @ingroup Executor
 */
class CreateTableExecutor
{
public:
  CreateTableExecutor()          = default;
  virtual ~CreateTableExecutor() = default;

  /**
   * @brief 执行 `CREATE TABLE`。
   * @param sql_event 当前 SQL 请求上下文。
   * @return 返回底层建表接口的执行结果。
   */
  RC execute(SQLStageEvent *sql_event);
};
