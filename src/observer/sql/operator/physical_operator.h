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
// Created by WangYunlai on 2022/6/7.
//

#pragma once

#include "common/sys/rc.h"
#include "sql/expr/tuple.h"
#include "sql/operator/operator_node.h"

class Record;
class TupleCellSpec;
class Trx;

/**
 * @file physical_operator.h
 * @brief 物理执行计划节点抽象。
 * @details 物理算子描述“如何执行”，负责真正拉取 tuple/chunk、维护运行时状态，
 * 并以树形结构组成执行器。
 */

/**
 * @brief 物理算子
 * @defgroup PhysicalOperator
 * @details 物理算子描述执行计划将如何执行，比如从表中怎么获取数据，如何做投影，怎么做连接等
 */

/**
 * @brief 物理算子类型
 * @ingroup PhysicalOperator
 */
enum class PhysicalOperatorType
{
  TABLE_SCAN,
  TABLE_SCAN_VEC,
  INDEX_SCAN,
  NESTED_LOOP_JOIN,
  HASH_JOIN,
  EXPLAIN,
  PREDICATE,
  PREDICATE_VEC,
  PROJECT,
  PROJECT_VEC,
  CALC,
  STRING_LIST,
  DELETE,
  INSERT,
  SCALAR_GROUP_BY,
  HASH_GROUP_BY,
  GROUP_BY_VEC,
  AGGREGATE_VEC,
  EXPR_VEC,
};

/**
 * @brief 与LogicalOperator对应，物理算子描述执行计划将如何执行
 * @ingroup PhysicalOperator
 */
class PhysicalOperator : public OperatorNode
{
public:
  PhysicalOperator() = default;

  virtual ~PhysicalOperator() = default;

  /**
   * 这两个函数是为了打印时使用的，比如在explain中
   */
  virtual string name() const;
  virtual string param() const;

  bool is_physical() const override { return true; }
  bool is_logical() const override { return false; }

  virtual PhysicalOperatorType type() const = 0;

  /**
   * @brief 打开算子并初始化运行时资源。
   * @details 执行器在首次拉取数据前调用，通常会级联打开子算子。
   */
  virtual RC open(Trx *trx) = 0;
  /// @brief 逐行执行接口，返回下一条 tuple 是否可用。
  virtual RC next() { return RC::UNIMPLEMENTED; }
  /// @brief 向量化执行接口，返回下一块 chunk 是否可用。
  virtual RC next(Chunk &chunk) { return RC::UNIMPLEMENTED; }
  /// @brief 关闭算子并释放运行时资源。
  virtual RC close() = 0;

  /// @brief 返回最近一次 `next()` 产生的当前 tuple。
  virtual Tuple *current_tuple() { return nullptr; }

  /// @brief 返回算子对外暴露的输出 schema。
  virtual RC tuple_schema(TupleSchema &schema) const { return RC::UNIMPLEMENTED; }

  /// @brief 追加一个物理子算子并转移所有权。
  void add_child(unique_ptr<PhysicalOperator> oper) { children_.emplace_back(std::move(oper)); }

  vector<unique_ptr<PhysicalOperator>> &children() { return children_; }

protected:
  vector<unique_ptr<PhysicalOperator>> children_;
};
