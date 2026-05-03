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
// Created by Longda on 2021/4/14.
//

#pragma once

#include "common/lang/string.h"
#include "common/lang/memory.h"
#include "sql/operator/physical_operator.h"

class SessionEvent;
class Stmt;
class ParsedSqlNode;

/**
 * @brief SQL 处理流水线中的中间事件对象。
 * @details 它会在 parse、resolve、optimize、execute 等阶段之间传递 SQL 文本、
 * 语法树、Stmt 和物理计划，避免各阶段重复解析或复制状态。
 */
class SQLStageEvent
{
public:
  /**
   * @brief 构造一条 SQL 的阶段事件。
   * @param event 发起本次 SQL 的顶层 SessionEvent，不能为空。
   * @param sql 当前待处理的 SQL 文本。
   */
  SQLStageEvent(SessionEvent *event, const string &sql);

  /**
   * @brief 析构阶段事件并释放其拥有的中间对象。
   * @note stmt_ 仍然使用裸指针约定，因此在析构函数中显式 delete。
   */
  virtual ~SQLStageEvent() noexcept;

  /// @brief 返回顶层请求事件。
  SessionEvent *session_event() const { return session_event_; }

  /// @brief 返回当前阶段正在处理的 SQL 文本。
  const string                       &sql() const { return sql_; }
  /// @brief 返回 parser 生成的语法树。
  const unique_ptr<ParsedSqlNode>    &sql_node() const { return sql_node_; }
  /// @brief 返回 resolver 生成的语义化语句对象。
  Stmt                               *stmt() const { return stmt_; }
  /// @brief 返回可修改的物理执行计划句柄。
  unique_ptr<PhysicalOperator>       &physical_operator() { return operator_; }
  /// @brief 返回只读的物理执行计划句柄。
  const unique_ptr<PhysicalOperator> &physical_operator() const { return operator_; }

  /// @brief 用新的 SQL 文本覆盖当前事件中的语句。
  void set_sql(const char *sql) { sql_ = sql; }
  /// @brief 注入 parser 生成的语法树所有权。
  void set_sql_node(unique_ptr<ParsedSqlNode> sql_node) { sql_node_ = std::move(sql_node); }
  /// @brief 设置 resolver 生成的 Stmt；事件析构时负责释放。
  void set_stmt(Stmt *stmt) { stmt_ = stmt; }
  /// @brief 注入优化器或执行器生成的物理算子树所有权。
  void set_operator(unique_ptr<PhysicalOperator> oper) { operator_ = std::move(oper); }

private:
  SessionEvent                *session_event_ = nullptr;
  string                       sql_;             ///< 处理的SQL语句
  unique_ptr<ParsedSqlNode>    sql_node_;        ///< 语法解析后的SQL命令
  Stmt                        *stmt_ = nullptr;  ///< Resolver之后生成的数据结构
  unique_ptr<PhysicalOperator> operator_;        ///< 生成的执行计划，也可能没有
};
