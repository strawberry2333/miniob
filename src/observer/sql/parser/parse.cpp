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

#include "sql/parser/parse.h"
#include "common/log/log.h"
#include "sql/expr/expression.h"

RC parse(char *st, ParsedSqlNode *sqln);

/**
 * @file parse.cpp
 * @brief 实现 SQL 解析层的对外包装。
 */

ParsedSqlNode::ParsedSqlNode() : flag(SCF_ERROR) {}

ParsedSqlNode::ParsedSqlNode(SqlCommandFlag _flag) : flag(_flag) {}

void ParsedSqlResult::add_sql_node(unique_ptr<ParsedSqlNode> sql_node)
{
  // 解析层按出现顺序收集语句节点；上游当前只消费第一条。
  sql_nodes_.emplace_back(std::move(sql_node));
}

////////////////////////////////////////////////////////////////////////////////

int sql_parse(const char *st, ParsedSqlResult *sql_result);

RC parse(const char *st, ParsedSqlResult *sql_result)
{
  // 真正的词法/语法分析由 flex+bison 生成的 `sql_parse` 完成。
  sql_parse(st, sql_result);
  return RC::SUCCESS;
}
