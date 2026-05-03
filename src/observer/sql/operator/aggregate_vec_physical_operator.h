/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#pragma once

#include "sql/operator/physical_operator.h"

/**
 * @file aggregate_vec_physical_operator.h
 * @brief 无分组列时的向量化聚合算子。
 */

/**
 * @brief 聚合物理算子 (Vectorized)
 * @ingroup PhysicalOperator
 */
class AggregateVecPhysicalOperator : public PhysicalOperator
{
public:
  AggregateVecPhysicalOperator(vector<Expression *> &&expressions);

  virtual ~AggregateVecPhysicalOperator() = default;

  PhysicalOperatorType type() const override { return PhysicalOperatorType::AGGREGATE_VEC; }

  RC open(Trx *trx) override;
  /// @brief 该算子只会产出一个包含聚合结果的 chunk。
  RC next(Chunk &chunk) override;
  RC close() override;

private:
  template <class STATE, typename T>
  void update_aggregate_state(void *state, const Column &column);

private:
  class AggregateValues
  {
  public:
    AggregateValues() = default;

    /// @brief 保存一个聚合状态对象的裸指针。
    void insert(void *aggr_value) { data_.push_back(aggr_value); }

    void *at(size_t index)
    {
      ASSERT(index <= data_.size(), "index out of range");
      return data_[index];
    }

    size_t size() { return data_.size(); }
    ~AggregateValues()
    {
      for (auto &aggr_value : data_) {
        free(aggr_value);
        aggr_value = nullptr;
      }
    }

  private:
    vector<void *> data_;
  };
  vector<Expression *> aggregate_expressions_;  /// 聚合表达式
  vector<Expression *> value_expressions_;
  Chunk                chunk_;
  Chunk                output_chunk_;
  AggregateValues      aggr_values_;
  bool                 outputed_ = false;
};
