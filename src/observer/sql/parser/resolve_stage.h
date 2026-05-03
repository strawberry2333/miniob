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
 * @file resolve_stage.h
 * @brief 定义 SQL 流水线中的 resolve 阶段。
 */

/**
 * @brief 执行Resolve，将解析后的SQL语句，转换成各种Stmt(Statement), 同时会做错误检查
 * @ingroup SQLStage
 */
class ResolveStage
{
public:
  /**
   * @brief 把 `ParsedSqlNode` 转换为绑定后的 `Stmt`。
   * @param sql_event 当前 SQL 请求上下文。
   * @return 返回解析绑定结果。
   */
  RC handle_request(SQLStageEvent *sql_event);
};
