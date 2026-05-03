/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "sql/expr/expression.h"
#include "common/type/attr_type.h"

/**
 * @file aggregate_state.h
 * @brief 向量化聚合执行路径使用的裸状态定义。
 * @details `AggregateVecPhysicalOperator` 与 `AggregateHashTable` 不直接复用
 * `Aggregator`，而是通过模板状态对象配合 `void *` 分发，尽量减少逐行虚调用。
 */

/**
 * @brief `SUM` 的向量化聚合状态。
 * @tparam T 聚合输入的底层 C++ 类型。
 */
template <class T>
class SumState
{
public:
  SumState() : value(0) {}
  T    value;

  /**
   * @brief 批量累加一列数据。
   * @param values 列数据起始地址。
   * @param size 待聚合元素个数。
   */
  void update(const T *values, int size);

  /**
   * @brief 吸收一条标量输入。
   * @details 供哈希聚合逐行更新状态时复用。
   */
  void update(const T &value) { this->value += value; }

  /**
   * @brief 将内部状态转换为输出类型。
   * @tparam U 输出列的目标类型。
   */
  template <class U>
  U finalize()
  {
    return (U)value;
  }
};

/**
 * @brief `COUNT` 的向量化聚合状态。
 * @details 当前实现不关心输入值内容，只关心行数。
 */
template <class T>
class CountState
{
public:
  CountState() : value(0) {}
  int  value;
  void update(const T *values, int size);
  void update(const T &value) { this->value++; }
  template <class U>
  U finalize()
  {
    return (U)value;
  }
};

/**
 * @brief `AVG` 的向量化聚合状态。
 * @details 内部同时维护累计和与计数，最终在 `finalize` 阶段做一次除法。
 */
template <class T>
class AvgState
{
public:
  AvgState() : value(0), count(0) {}
  T    value;
  int  count = 0;
  void update(const T *values, int size);
  void update(const T &value)
  {
    this->value += value;
    this->count++;
  }
  template <class U>
  U finalize()
  {
    return (U)((float)value / (float)count);
  }
};

/**
 * @brief 根据聚合函数类型和输入类型创建聚合状态。
 * @return 返回经 placement new 初始化后的状态指针；不支持时返回 `nullptr`。
 * @note 调用方负责在生命周期结束时释放返回的内存。
 */
void *create_aggregate_state(AggregateExpr::Type aggr_type, AttrType attr_type);

/**
 * @brief 用一条标量值更新聚合状态。
 * @details 哈希聚合逐行吸收输入时由此入口完成类型分发。
 */
RC aggregate_state_update_by_value(void *state, AggregateExpr::Type aggr_type, AttrType attr_type, const Value &val);

/**
 * @brief 用一列数据批量更新聚合状态。
 * @details 标量聚合的向量化版本会在读取 `Chunk` 后走这个入口。
 */
RC aggregate_state_update_by_column(void *state, AggregateExpr::Type aggr_type, AttrType attr_type, Column &col);

/**
 * @brief 将聚合状态写回结果列。
 * @details 该函数会调用具体状态的 `finalize`，再把结果追加到输出列末尾。
 */
RC finialize_aggregate_state(void *state, AggregateExpr::Type aggr_type, AttrType attr_type, Column &col);
