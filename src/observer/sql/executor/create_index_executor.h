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
 * @file create_index_executor.h
 * @brief 定义 `CREATE INDEX` 命令执行器。
 */

/**
 * @brief 创建索引的命令执行器。
 * @ingroup Executor
 * @note 创建索引时不能做其它操作。MiniOB 当前没有完整的 schema 并发控制。
 */
class CreateIndexExecutor
{
public:
  CreateIndexExecutor()          = default;
  virtual ~CreateIndexExecutor() = default;

  /**
   * @brief 执行 `CREATE INDEX`。
   * @param sql_event 当前 SQL 请求上下文。
   * @return 返回底层 `Table::create_index` 的执行结果。
   */
  RC execute(SQLStageEvent *sql_event);
};
