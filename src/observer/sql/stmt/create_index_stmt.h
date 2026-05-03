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
// Created by Wangyunlai on 2023/4/25.
//

#pragma once

#include "sql/stmt/stmt.h"

struct CreateIndexSqlNode;
class Table;
class FieldMeta;

/**
 * @file create_index_stmt.h
 * @brief 定义 `CREATE INDEX` 的语义对象。
 */

/**
 * @brief 表示已经完成表和字段绑定的创建索引语句。
 * @ingroup Statement
 */
class CreateIndexStmt : public Stmt
{
public:
  /**
   * @brief 构造创建索引语句。
   * @param table 目标表。
   * @param field_meta 需要建立索引的字段元数据。
   * @param index_name 索引名。
   */
  CreateIndexStmt(Table *table, const FieldMeta *field_meta, const string &index_name)
      : table_(table), field_meta_(field_meta), index_name_(index_name)
  {}

  virtual ~CreateIndexStmt() = default;

  StmtType type() const override { return StmtType::CREATE_INDEX; }

  /// 返回目标表。
  Table           *table() const { return table_; }
  /// 返回目标字段元数据。
  const FieldMeta *field_meta() const { return field_meta_; }
  /// 返回索引名。
  const string    &index_name() const { return index_name_; }

public:
  /**
   * @brief 从 parse 节点构造并校验创建索引语句。
   * @param db 当前数据库。
   * @param create_index parse 阶段的创建索引节点。
   * @param stmt 输出语句对象。
   * @return 返回 schema 校验结果。
   */
  static RC create(Db *db, const CreateIndexSqlNode &create_index, Stmt *&stmt);

private:
  Table           *table_      = nullptr;  ///< 目标表
  const FieldMeta *field_meta_ = nullptr;  ///< 目标字段
  string           index_name_;            ///< 索引名
};
