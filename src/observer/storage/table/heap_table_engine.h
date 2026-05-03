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

class Table;
/**
 * @file heap_table_engine.h
 * @brief 定义基于页式记录管理器和 B+ 树索引的 Heap 表引擎。
 */

/**
 * @brief Heap 表引擎。
 * @details 负责把表级操作拆分为：
 * 1. 数据页上的记录读写；
 * 2. 关联索引项维护；
 * 3. 扫描器和页面访问器构造。
 */
class HeapTableEngine : public TableEngine
{
public:
  friend class Table;
  HeapTableEngine(TableMeta *table_meta, Db *db, Table *table) : TableEngine(table_meta), db_(db), table_(table) {}
  ~HeapTableEngine() override;

  /// @brief 插入一条记录，并在成功后同步维护所有索引。
  RC insert_record(Record &record) override;
  /// @brief 批量插入一批列式数据；当前仅更新数据文件，不回填索引。
  RC insert_chunk(const Chunk &chunk) override;
  /// @brief 删除记录及其索引项。
  RC delete_record(const Record &record) override;
  RC insert_record_with_trx(Record &record, Trx *trx) override { return RC::UNSUPPORTED; }
  RC delete_record_with_trx(const Record &record, Trx *trx) override { return RC::UNSUPPORTED; }
  RC update_record_with_trx(const Record &old_record, const Record &new_record, Trx *trx) override
  {
    return RC::UNSUPPORTED;
  }
  /// @brief 按 RID 获取一条记录。
  RC get_record(const RID &rid, Record &record) override;

  /// @brief 为指定字段创建新索引，并回填历史数据。
  RC create_index(Trx *trx, const FieldMeta *field_meta, const char *index_name) override;
  /// @brief 创建面向行记录的文件扫描器。
  RC get_record_scanner(RecordScanner *&scanner, Trx *trx, ReadWriteMode mode) override;
  /// @brief 创建批量 `Chunk` 扫描器。
  RC get_chunk_scanner(ChunkFileScanner &scanner, Trx *trx, ReadWriteMode mode) override;
  /// @brief 在持锁上下文中访问指定记录。
  RC visit_record(const RID &rid, function<bool(Record &)> visitor) override;
  /// @brief 将数据页和索引页的脏数据全部刷盘。
  RC sync() override;

  Index *find_index(const char *index_name) const override;
  Index *find_index_by_field(const char *field_name) const override;
  RC     open() override;
  /// @brief 打开数据文件、初始化记录处理器并加载索引。
  RC init() override;

private:
  /// @brief 向全部索引追加一条索引项；任一失败都由调用方决定是否回滚记录。
  RC insert_entry_of_indexes(const char *record, const RID &rid);
  /// @brief 从全部索引删除一条索引项，可按需忽略“不存在”错误。
  RC delete_entry_of_indexes(const char *record, const RID &rid, bool error_on_not_exists);

private:
  DiskBufferPool    *data_buffer_pool_ = nullptr;  /// 数据文件关联的buffer pool
  RecordFileHandler *record_handler_   = nullptr;  /// 记录操作
  vector<Index *>    indexes_;
  Db                *db_;
  Table             *table_;
};
