/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#pragma once

#include "storage/table/table_engine.h"
#include "storage/index/index.h"
#include "storage/record/record_manager.h"
#include "storage/db/db.h"
#include "oblsm/include/ob_lsm.h"
using namespace oceanbase;

/**
 * @file lsm_table_engine.h
 * @brief 定义对接 ObLsm 的 LSM 表引擎。
 */

/**
 * @brief LSM 表引擎。
 * @details 当前实现仍在演进中，主要承担主数据写入和 LSM 扫描器构造，
 * 大量事务/索引能力仍保持 `UNIMPLEMENTED`。
 */
class LsmTableEngine : public TableEngine
{
public:
  LsmTableEngine(TableMeta *table_meta, Db *db, Table *table)
      : TableEngine(table_meta), db_(db), table_(table), lsm_(db->lsm())
  {}
  ~LsmTableEngine() override = default;

  /// @brief 将记录编码成 LSM key/value 后写入底层 ObLsm。
  RC insert_record(Record &record) override;
  RC insert_chunk(const Chunk &chunk) override { return RC::UNIMPLEMENTED; }
  RC delete_record(const Record &record) override { return RC::UNIMPLEMENTED; }
  RC insert_record_with_trx(Record &record, Trx *trx) override { return RC::UNIMPLEMENTED; }
  RC delete_record_with_trx(const Record &record, Trx *trx) override { return RC::UNIMPLEMENTED; }
  RC update_record_with_trx(const Record &old_record, const Record &new_record, Trx *trx) override
  {
    return RC::UNIMPLEMENTED;
  }
  RC get_record(const RID &rid, Record &record) override { return RC::UNIMPLEMENTED; }

  RC create_index(Trx *trx, const FieldMeta *field_meta, const char *index_name) override { return RC::UNIMPLEMENTED; }
  /// @brief 创建基于 ObLsm 迭代器的记录扫描器。
  RC get_record_scanner(RecordScanner *&scanner, Trx *trx, ReadWriteMode mode) override;
  RC get_chunk_scanner(ChunkFileScanner &scanner, Trx *trx, ReadWriteMode mode) override { return RC::UNIMPLEMENTED; }
  RC visit_record(const RID &rid, function<bool(Record &)> visitor) override { return RC::UNIMPLEMENTED; }
  // TODO:
  RC     sync() override { return RC::SUCCESS; }
  Index *find_index(const char *index_name) const override { return nullptr; }
  Index *find_index_by_field(const char *field_name) const override { return nullptr; }
  /// @brief 打开已有 LSM 表。目前尚未完整实现元数据恢复逻辑。
  RC     open() override;
  RC     init() override { return RC::UNIMPLEMENTED; }

private:
  Db              *db_;
  Table           *table_;
  ObLsm           *lsm_;
  atomic<uint64_t> inc_id_{0};
};
