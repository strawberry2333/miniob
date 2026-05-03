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

class SQLStageEvent;

/**
 * @file desc_table_executor.h
 * @brief 定义 `DESC TABLE` 命令执行器。
 */

/**
 * @brief 输出表结构信息的命令执行器。
 * @ingroup Executor
 */
class DescTableExecutor
{
public:
  DescTableExecutor()          = default;
  virtual ~DescTableExecutor() = default;

  /**
   * @brief 执行 `DESC table_name`。
   * @param sql_event 当前 SQL 请求上下文。
   * @return 返回表结构构造结果；字段列表通过 `SqlResult` 返回给客户端。
   */
  RC execute(SQLStageEvent *sql_event);
};
