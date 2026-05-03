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
// Created by Wangyunlai on 2022/12/07.
//

#pragma once

#include "sql/expr/expression.h"
#include "sql/operator/operator_node.h"
#include "common/lang/unordered_set.h"

/**
 * @file logical_operator.h
 * @brief 逻辑计划树的基础抽象。
 * @details 逻辑算子只描述“要做什么”，例如扫描、过滤、投影、连接与聚合，
 * 不关心具体执行算法。优化器会在该层做规则改写，随后再映射到物理算子。
 */

/**
 * @brief 逻辑算子
 * @defgroup LogicalOperator
 * @details 逻辑算子描述当前执行计划要做什么，比如从表中获取数据，过滤，投影，连接等等。
 * 物理算子会描述怎么做某件事情，这是与其不同的地方。
 */

/**
 * @brief 逻辑算子类型
 *
 */
enum class LogicalOperatorType
{
  CALC,
  TABLE_GET,   ///< 从表中获取数据
  PREDICATE,   ///< 过滤，就是谓词
  PROJECTION,  ///< 投影，就是select
  JOIN,        ///< 连接
  INSERT,      ///< 插入
  DELETE,      ///< 删除，删除可能会有子查询
  EXPLAIN,     ///< 查看执行计划
  GROUP_BY,    ///< 分组
};

/**
 * @brief 逻辑算子描述当前执行计划要做什么
 * @details 可以看OptimizeStage中相关的代码
 */
class LogicalOperator : public OperatorNode
{
public:
  LogicalOperator() = default;
  virtual ~LogicalOperator();

  virtual LogicalOperatorType type() const = 0;

  bool is_physical() const override { return false; }
  bool is_logical() const override { return true; }

  /**
   * @brief 追加一个逻辑子算子。
   * @details 逻辑计划生成器和重写器都会用它拼接树结构，所有权会转移到当前节点。
   */
  void        add_child(unique_ptr<LogicalOperator> oper);

  /**
   * @brief 追加一个挂在当前算子上的表达式。
   * @details 不同算子对表达式的语义不同，例如投影列、谓词、分组列等。
   */
  void        add_expressions(unique_ptr<Expression> expr);
  auto        children() -> vector<unique_ptr<LogicalOperator>>        &{ return children_; }
  auto        expressions() -> vector<unique_ptr<Expression>>        &{ return expressions_; }

  /**
   * @brief 判断该逻辑算子是否存在向量化物理实现。
   * @details `OptimizeStage::generate_physical_plan` 会用它决定是否走 chunk 模式。
   */
  static bool can_generate_vectorized_operator(const LogicalOperatorType &type);
  // TODO: used by cascade optimizer, tmp function, need to be remove

  /**
   * @brief 为级联优化器生成统一的子节点数组。
   * @details `OperatorNode::general_children_` 允许逻辑/物理节点在 memo 中走同一套遍历逻辑。
   */
  void generate_general_child();

protected:
  vector<unique_ptr<LogicalOperator>> children_;  ///< 子算子

  ///< 表达式，比如select中的列，where中的谓词等等，都可以使用表达式来表示
  ///< 表达式能是一个常量，也可以是一个函数，也可以是一个列，也可以是一个子查询等等
  vector<unique_ptr<Expression>> expressions_;
};
