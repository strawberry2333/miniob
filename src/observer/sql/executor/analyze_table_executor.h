/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#pragma once

#include "common/sys/rc.h"

class SQLStageEvent;
class RecordScanner;

/**
 * @file analyze_table_executor.h
 * @brief 定义 `ANALYZE TABLE` 命令的执行器。
 */

/**
 * @brief 分析表统计信息的命令执行器。
 * @ingroup Executor
 * @details 当前实现通过全表扫描估算行数，然后把结果回写到 `Catalog`。
 * 这个实现成本较高，但能完整串起一条从命令到统计信息更新的控制流。
 */
class AnalyzeTableExecutor
{
public:
  AnalyzeTableExecutor() = default;
  /**
   * @brief 释放执行过程中申请的扫描器。
   */
  virtual ~AnalyzeTableExecutor();

  /**
   * @brief 执行 `ANALYZE TABLE`。
   * @param sql_event 当前 SQL 请求上下文，包含语句、会话和返回结果对象。
   * @return 返回分析执行结果；表不存在等错误会同步写回 `SqlResult`。
   */
  RC execute(SQLStageEvent *sql_event);

private:
  /// 保存表扫描器，便于在执行结束后统一释放。
  RecordScanner *scanner_ = nullptr;
};
