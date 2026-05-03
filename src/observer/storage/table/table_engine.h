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

#include "common/types.h"
#include "common/lang/functional.h"
#include "storage/table/table_meta.h"
#include "storage/common/chunk.h"

struct RID;
class Record;
class DiskBufferPool;
class RecordFileHandler;
class RecordScanner;
class ChunkFileScanner;
class ConditionFilter;
class DefaultConditionFilter;
class Index;
class IndexScanner;
class RecordDeleter;
class Trx;
class Db;

/**
 * @file table_engine.h
 * @brief 定义表引擎统一接口。
 */

/**
 * @brief 表引擎抽象基类。
 * @details 不同存储引擎通过实现该接口接入 `Table`，从而对上层暴露一致的 CRUD、
 * 扫描、索引管理和同步语义。
 */
class TableEngine
{
public:
  TableEngine(TableMeta *table_meta) : table_meta_(table_meta) {}
  virtual ~TableEngine() = default;

  /// @brief 插入一条记录。
  virtual RC insert_record(Record &record)                                                        = 0;
  /// @brief 批量插入一批列式数据。
  virtual RC insert_chunk(const Chunk &chunk)                                                     = 0;
  /// @brief 删除一条记录。
  virtual RC delete_record(const Record &record)                                                  = 0;
  virtual RC insert_record_with_trx(Record &record, Trx *trx)                                     = 0;
  virtual RC delete_record_with_trx(const Record &record, Trx *trx)                               = 0;
  virtual RC update_record_with_trx(const Record &old_record, const Record &new_record, Trx *trx) = 0;
  virtual RC get_record(const RID &rid, Record &record)                                           = 0;

  virtual RC     create_index(Trx *trx, const FieldMeta *field_meta, const char *index_name) = 0;
  virtual RC     get_record_scanner(RecordScanner *&scanner, Trx *trx, ReadWriteMode mode)   = 0;
  virtual RC     get_chunk_scanner(ChunkFileScanner &scanner, Trx *trx, ReadWriteMode mode)  = 0;
  virtual RC     visit_record(const RID &rid, function<bool(Record &)> visitor)              = 0;
  virtual RC     sync()                                                                      = 0;
  virtual Index *find_index(const char *index_name) const                                    = 0;
  virtual Index *find_index_by_field(const char *field_name) const                           = 0;
  virtual RC     open()                                                                      = 0;
  /// @brief 旧初始化入口，仍被现有表创建流程依赖。
  virtual RC init() = 0;

protected:
  TableMeta *table_meta_ = nullptr;
};
