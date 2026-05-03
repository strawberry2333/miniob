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
// Created by Meiyi
//

#pragma once

#include "common/sys/rc.h"
#include "sql/parser/parse_defs.h"

/**
 * @file parse.h
 * @brief 声明 SQL 文本到 `ParsedSqlResult` 的解析入口。
 */

/**
 * @brief 对输入 SQL 进行词法和语法分析。
 * @param st 原始 SQL 字符串。
 * @param sql_result 解析结果容器，内部会追加一个或多个 `ParsedSqlNode`。
 * @return 当前实现统一返回 `RC::SUCCESS`，语法错误通过 `sql_result` 中的错误节点表达。
 */
RC parse(const char *st, ParsedSqlResult *sql_result);
