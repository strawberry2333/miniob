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
// Created by Wangyunlai on 2024/05/29.
//

#pragma once

#include "sql/expr/expression.h"

/**
 * @file expression_binder.h
 * @brief 定义 SELECT/CALC 等表达式的绑定器。
 */

/**
 * @brief 保存表达式绑定时可见的查询上下文。
 * @details 当前上下文只维护 `FROM` 子句中的表列表，后续可以扩展别名、子查询或列作用域信息。
 */
class BinderContext
{
public:
  BinderContext()          = default;
  virtual ~BinderContext() = default;

  /**
   * @brief 注册当前查询可见的一张表。
   * @param table 参与本次查询的表对象。
   */
  void add_table(Table *table) { query_tables_.push_back(table); }

  /**
   * @brief 按名字查找当前查询范围内的表。
   * @param table_name 表名。
   * @return 找到则返回表指针，否则返回 `nullptr`。
   */
  Table *find_table(const char *table_name) const;

  /**
   * @brief 返回当前查询范围内的所有表。
   */
  const vector<Table *> &query_tables() const { return query_tables_; }

private:
  vector<Table *> query_tables_;
};

/**
 * @brief 绑定表达式
 * @details 绑定表达式，就是在SQL解析后，得到文本描述的表达式，将表达式解析为具体的数据库对象
 */
class ExpressionBinder
{
public:
  ExpressionBinder(BinderContext &context) : context_(context) {}
  virtual ~ExpressionBinder() = default;

  /**
   * @brief 绑定一棵表达式子树。
   * @param expr 输入表达式，可能是未绑定字段、星号或未绑定聚合等。
   * @param bound_expressions 输出绑定结果。之所以是列表，是因为 `*` 可能展开成多列表达式。
   * @return 返回绑定结果。
   */
  RC bind_expression(unique_ptr<Expression> &expr, vector<unique_ptr<Expression>> &bound_expressions);

private:
  /// 绑定 `*` 或 `table.*`。
  RC bind_star_expression(unique_ptr<Expression> &star_expr, vector<unique_ptr<Expression>> &bound_expressions);
  /// 绑定尚未解析到具体表字段的列引用。
  RC bind_unbound_field_expression(
      unique_ptr<Expression> &unbound_field_expr, vector<unique_ptr<Expression>> &bound_expressions);
  /// 绑定已经解析完成的字段表达式，直接透传即可。
  RC bind_field_expression(unique_ptr<Expression> &field_expr, vector<unique_ptr<Expression>> &bound_expressions);
  /// 绑定值表达式，直接透传即可。
  RC bind_value_expression(unique_ptr<Expression> &value_expr, vector<unique_ptr<Expression>> &bound_expressions);
  /// 绑定 CAST 表达式及其子节点。
  RC bind_cast_expression(unique_ptr<Expression> &cast_expr, vector<unique_ptr<Expression>> &bound_expressions);
  /// 绑定比较表达式及左右子树。
  RC bind_comparison_expression(
      unique_ptr<Expression> &comparison_expr, vector<unique_ptr<Expression>> &bound_expressions);
  /// 绑定逻辑与/或表达式的所有子节点。
  RC bind_conjunction_expression(
      unique_ptr<Expression> &conjunction_expr, vector<unique_ptr<Expression>> &bound_expressions);
  /// 绑定算术表达式的左右子树。
  RC bind_arithmetic_expression(
      unique_ptr<Expression> &arithmetic_expr, vector<unique_ptr<Expression>> &bound_expressions);
  /// 绑定聚合表达式，并完成聚合函数名和参数校验。
  RC bind_aggregate_expression(
      unique_ptr<Expression> &aggregate_expr, vector<unique_ptr<Expression>> &bound_expressions);

private:
  /// 绑定过程中共享的查询可见性上下文。
  BinderContext &context_;
};
