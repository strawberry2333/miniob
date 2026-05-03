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
// Created by Wangyunlai on 2023/7/12.
//

#pragma once

#include "common/sys/rc.h"

class SQLStageEvent;
class Table;
class SqlResult;

/**
 * @file load_data_executor.h
 * @brief 定义 `LOAD DATA` 命令执行器。
 */

/**
 * @brief 从外部文件批量导入数据的命令执行器。
 * @ingroup Executor
 */
class LoadDataExecutor
{
public:
  LoadDataExecutor()          = default;
  virtual ~LoadDataExecutor() = default;

  /**
   * @brief 执行 `LOAD DATA`。
   * @param sql_event 当前 SQL 请求上下文。
   * @return 返回调度结果；具体导入状态通过 `SqlResult` 汇报。
   */
  RC execute(SQLStageEvent *sql_event);

private:
  /**
   * @brief 将文件逐行解析并插入到目标表。
   * @param table 目标表。
   * @param file_name 数据文件路径。
   * @param terminated 字段分隔符配置，当前实现仅保留接口并未完全消费。
   * @param enclosed 文本包裹符配置，当前实现仅保留接口并未完全消费。
   * @param sql_result 用于回写执行状态和错误信息。
   */
  void load_data(Table *table, const char *file_name, char terminated, char enclosed, SqlResult *sql_result);
};
