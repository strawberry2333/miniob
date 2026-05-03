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

#include "common/sys/rc.h"
#include "common/log/log.h"
#include "common/lang/memory.h"
#include "common/lang/vector.h"
#include "storage/common/column.h"

/**
 * @file chunk.h
 * @brief 定义向量化执行使用的列批容器 `Chunk`。
 * @details `Chunk` 将一批行按列组织，便于列式扫描、批量表达式计算和 PAX/向量化接口传递。
 */

/**
 * @brief 表示一批按列组织的数据。
 * @details `Chunk` 本身不解释列值语义，只负责维护多列对齐的行数与列标识。
 * 大多数场景下每一列容量相同，因此 `rows/capacity` 直接以第 0 列为准。
 */
class Chunk
{
public:
  static const int MAX_ROWS = Column::DEFAULT_CAPACITY;
  Chunk()                   = default;
  Chunk(const Chunk &other)
  {
    for (size_t i = 0; i < other.columns_.size(); ++i) {
      columns_.emplace_back(other.columns_[i]->clone());
    }
    column_ids_ = other.column_ids_;
  }
  Chunk(Chunk &&chunk)
  {
    columns_    = std::move(chunk.columns_);
    column_ids_ = std::move(chunk.column_ids_);
  }

  int column_num() const { return columns_.size(); }

  Column &column(size_t idx)
  {
    ASSERT(idx < columns_.size(), "invalid column index");
    return *columns_[idx];
  }

  const Column &column(size_t idx) const
  {
    ASSERT(idx < columns_.size(), "invalid column index");
    return *columns_[idx];
  }

  Column *column_ptr(size_t idx)
  {
    ASSERT(idx < columns_.size(), "invalid column index");
    return &column(idx);
  }

  int column_ids(size_t i)
  {
    ASSERT(i < column_ids_.size(), "invalid column index");
    return column_ids_[i];
  }

  /**
   * @brief 向批中追加一列。
   * @param col 列对象所有权会移动进 `Chunk`。
   * @param col_id 逻辑列编号，供上层算子决定输出列映射。
   */
  void add_column(unique_ptr<Column> col, int col_id);

  /**
   * @brief 将当前 Chunk 改为引用另一个 Chunk 的列数据。
   * @details 该操作不会深拷贝列内容，而是复用对方列缓冲区；调用方需要保证源 `chunk`
   * 在引用期间保持存活且不提前释放底层列数据。
   */
  RC reference(Chunk &chunk);

  /**
   * @brief 获取 Chunk 中的行数
   */
  int rows() const;

  /**
   * @brief 获取 Chunk 的容量
   */
  int capacity() const;

  /**
   * @brief 从 Chunk 中获得指定行指定列的 Value
   * @param col_idx 列索引
   * @param row_idx 行索引
   * @return Value
   * @note 没有检查 col_idx 和 row_idx 是否越界
   *
   */
  Value get_value(int col_idx, int row_idx) const { return columns_[col_idx]->get_value(row_idx); }

  /**
   * @brief 重置 Chunk 中的数据，不会修改 Chunk 的列属性。
   */
  void reset_data();

  /**
   * @brief 清空列描述与列 ID 映射。
   * @details 会释放 `Chunk` 对列对象的所有权，但不会主动清理外部被引用列的原始缓冲区。
   */
  void reset();

private:
  vector<unique_ptr<Column>> columns_;
  /// TODO: remove it and support multi-tables.
  /// `column_ids_` 记录上游算子输出列编号，当前仍由执行层依赖。
  vector<int> column_ids_;
};
