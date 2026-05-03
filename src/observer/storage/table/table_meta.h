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
// Created by Wangyunlai on 2021/5/12.
//

#pragma once

#include "common/lang/serializable.h"
#include "common/sys/rc.h"
#include "common/types.h"
#include "common/lang/span.h"
#include "storage/field/field_meta.h"
#include "storage/index/index_meta.h"

/**
 * @file table_meta.h
 * @brief 定义表级 schema、索引和存储格式元数据。
 */

/**
 * @brief 表元数据。
 * @details 该对象既承载逻辑 schema，也保存记录物理布局和索引定义，
 * 是表打开、恢复和 SQL 语义检查的共享事实来源。
 */
class TableMeta : public common::Serializable
{
public:
  TableMeta()          = default;
  virtual ~TableMeta() = default;

  TableMeta(const TableMeta &other);

  void swap(TableMeta &other) noexcept;

  /// @brief 按表定义初始化元数据并计算记录长度。
  RC init(int32_t table_id, const char *name, const vector<FieldMeta> *trx_fields,
      span<const AttrInfoSqlNode> attributes, const vector<string> &primary_keys, StorageFormat storage_format,
      StorageEngine storage_engine);

  /// @brief 向元数据中追加一个索引定义。
  RC add_index(const IndexMeta &index);

public:
  int32_t             table_id() const { return table_id_; }
  const char         *name() const;
  const FieldMeta    *trx_field() const;
  const FieldMeta    *field(int index) const;
  const FieldMeta    *field(const char *name) const;
  const FieldMeta    *find_field_by_offset(int offset) const;
  auto                field_metas() const -> const vector<FieldMeta>                *{ return &fields_; }
  auto                trx_fields() const -> span<const FieldMeta>;
  const StorageFormat storage_format() const { return storage_format_; }
  const StorageEngine storage_engine() const { return storage_engine_; }

  int field_num() const;  // sys field included
  int sys_field_num() const;

  const IndexMeta *index(const char *name) const;
  const IndexMeta *find_index_by_field(const char *field) const;
  const IndexMeta *index(int i) const;
  int              index_num() const;

  const vector<string> &primary_keys() const { return primary_keys_; }

  int record_size() const;

public:
  int  serialize(ostream &os) const override;
  int  deserialize(istream &is) override;
  int  get_serial_size() const override;
  void to_string(string &output) const override;
  void desc(ostream &os) const;

protected:
  int32_t           table_id_ = -1;
  string            name_;
  vector<FieldMeta> trx_fields_;
  vector<FieldMeta> fields_;  // 包含sys_fields
  vector<IndexMeta> indexes_;
  vector<string>    primary_keys_;
  StorageFormat     storage_format_;
  StorageEngine     storage_engine_;

  int record_size_ = 0;
};
