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

/**
 * @file query_cache_stage.h
 * @brief 定义查询缓存阶段。
 */

/**
 * @brief 查询缓存处理
 * @ingroup SQLStage
 * @details 当前仍是空实现，但类边界已经明确了“语句级结果缓存”的潜在接入点。
 */
class QueryCacheStage
{
public:
  QueryCacheStage()          = default;
  virtual ~QueryCacheStage() = default;

public:
  /**
   * @brief 尝试处理查询缓存请求。
   * @param sql_event 当前 SQL 请求上下文。
   * @return 当前总是返回 `RC::SUCCESS`，表示直接放行到后续阶段。
   */
  RC handle_request(SQLStageEvent *sql_event);
};
