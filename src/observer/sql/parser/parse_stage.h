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
 * @file parse_stage.h
 * @brief 定义 SQL 流水线中的 parse 阶段。
 */

/**
 * @brief 解析SQL语句，解析后的结果可以参考parse_defs.h
 * @ingroup SQLStage
 */
class ParseStage
{
public:
  /**
   * @brief 对当前请求中的 SQL 文本做 parse。
   * @param sql_event 当前 SQL 请求上下文。
   * @return 返回 parse 阶段状态；语法错误会写入 `SqlResult`。
   */
  RC handle_request(SQLStageEvent *sql_event);
};
