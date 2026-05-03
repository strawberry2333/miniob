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

#include "common/value.h"
#include "common/sys/rc.h"

/**
 * @file aggregator.h
 * @brief 行式聚合执行路径使用的聚合器接口。
 * @details `GroupByPhysicalOperator` 及其派生类会为每个聚合表达式创建一个
 * `Aggregator`，在逐行读取子算子结果时持续调用 `accumulate`，最终在输出阶段
 * 调用 `evaluate` 生成聚合结果。
 */

/**
 * @brief 聚合器抽象基类。
 * @details 子类负责维护某个聚合函数的内部状态。当前接口仅覆盖行式执行路径，
 * 向量化路径使用 `aggregate_state.*` 中的裸状态对象。
 */
class Aggregator
{
public:
  virtual ~Aggregator() = default;

  /**
   * @brief 吸收一行输入值并更新聚合状态。
   * @param value 当前行对应聚合表达式的值。
   * @return `RC::SUCCESS` 表示状态更新成功。
   * @note 调用方需要保证传入值与聚合器实现支持的类型一致。
   */
  virtual RC accumulate(const Value &value) = 0;

  /**
   * @brief 输出当前聚合状态对应的最终结果。
   * @param result 输出参数，返回聚合值。
   * @details `GroupByPhysicalOperator::evaluate` 会在所有输入处理结束后调用它。
   */
  virtual RC evaluate(Value &result)        = 0;

protected:
  Value value_;
};

/**
 * @brief `SUM` 的简单实现。
 * @details 首次吸收输入时直接以该值初始化状态，后续调用 `Value::add`
 * 做累加。当前只被 `AggregateExpr::create_aggregator` 的行式聚合路径使用。
 */
class SumAggregator : public Aggregator
{
public:
  RC accumulate(const Value &value) override;
  RC evaluate(Value &result) override;
};
