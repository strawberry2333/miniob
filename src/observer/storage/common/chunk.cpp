/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "storage/common/chunk.h"

/**
 * @file chunk.cpp
 * @brief `Chunk` 的列集合维护实现。
 */

void Chunk::add_column(unique_ptr<Column> col, int col_id)
{
  // 列对象所有权与逻辑列编号必须同步推进，保证列元信息和数据一一对应。
  columns_.push_back(std::move(col));
  column_ids_.push_back(col_id);
}

RC Chunk::reference(Chunk &chunk)
{
  // 引用模式下不保留旧列对象，避免旧列元数据与新引用数据混杂。
  reset();
  this->columns_.resize(chunk.column_num());
  for (size_t i = 0; i < columns_.size(); ++i) {
    if (nullptr == columns_[i]) {
      columns_[i] = make_unique<Column>();
    }
    columns_[i]->reference(chunk.column(i));
    column_ids_.push_back(chunk.column_ids(i));
  }
  return RC::SUCCESS;
}

int Chunk::rows() const
{
  // Chunk 约定所有列行数保持一致，因此直接以首列计数为准。
  if (!columns_.empty()) {
    return columns_[0]->count();
  }
  return 0;
}

int Chunk::capacity() const
{
  // 与 rows() 同理，容量也由首列代表整个批次的可容纳行数。
  if (!columns_.empty()) {
    return columns_[0]->capacity();
  }
  return 0;
}

void Chunk::reset_data()
{
  // 仅清空每列内容，保留列对象和列类型，便于批量复用。
  for (auto &col : columns_) {
    col->reset_data();
  }
}

void Chunk::reset()
{
  // 清空列集合后，Chunk 不再持有任何列所有权或输出列映射。
  columns_.clear();
  column_ids_.clear();
}
