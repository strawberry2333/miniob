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
#include "sql/operator/physical_operator.h"
#include "storage/record/record_manager.h"
#include "common/types.h"

class Table;

/**
 * @file table_scan_vec_physical_operator.h
 * @brief 向量化全表扫描算子。
 */

/**
 * @brief 表扫描物理算子(vectorized)
 * @ingroup PhysicalOperator
 */
class TableScanVecPhysicalOperator : public PhysicalOperator
{
public:
  TableScanVecPhysicalOperator(Table *table, ReadWriteMode mode) : table_(table), mode_(mode) {}

  virtual ~TableScanVecPhysicalOperator() = default;

  string param() const override;

  PhysicalOperatorType type() const override { return PhysicalOperatorType::TABLE_SCAN_VEC; }

  RC open(Trx *trx) override;
  /// @brief 扫描一块原始列并按选择向量过滤出满足条件的行。
  RC next(Chunk &chunk) override;
  RC close() override;

  void set_predicates(vector<unique_ptr<Expression>> &&exprs);

private:
  /// @brief 依次执行所有下推谓词，原地收窄 `select_` 选择向量。
  RC filter(Chunk &chunk);

private:
  Table                         *table_ = nullptr;
  ReadWriteMode                  mode_  = ReadWriteMode::READ_WRITE;
  ChunkFileScanner               chunk_scanner_;
  Chunk                          all_columns_;
  Chunk                          filterd_columns_;
  vector<uint8_t>                select_;
  vector<unique_ptr<Expression>> predicates_;
};
