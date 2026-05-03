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
// Created by Wangyunlai on 2023/6/13.
//

#pragma once

#include "common/lang/string.h"
#include "common/lang/vector.h"
#include "sql/stmt/stmt.h"

class Db;

/**
 * @file create_table_stmt.h
 * @brief 定义 `CREATE TABLE` 的语义对象。
 */

/**
 * @brief 表示创建表语句。
 * @ingroup Statement
 * @details 这一层主要把存储格式等文本参数规范化，字段定义仍保持接近 parse 结构。
 */
class CreateTableStmt : public Stmt
{
public:
  /**
   * @brief 构造建表语句对象。
   * @param table_name 目标表名。
   * @param attr_infos 字段定义列表。
   * @param pks 主键字段列表。
   * @param storage_format 存储格式枚举。
   */
  CreateTableStmt(const string &table_name, const vector<AttrInfoSqlNode> &attr_infos, const vector<string> &pks,
      StorageFormat storage_format)
      : table_name_(table_name), attr_infos_(attr_infos), primary_keys_(pks), storage_format_(storage_format)
  {}
  virtual ~CreateTableStmt() = default;

  StmtType type() const override { return StmtType::CREATE_TABLE; }

  /// 返回表名。
  const string                  &table_name() const { return table_name_; }
  /// 返回字段定义列表。
  const vector<AttrInfoSqlNode> &attr_infos() const { return attr_infos_; }
  /// 返回主键列表。
  const vector<string>          &primary_keys() const { return primary_keys_; }
  /// 返回建表使用的存储格式。
  const StorageFormat            storage_format() const { return storage_format_; }

  /**
   * @brief 从 parse 节点构造建表语句，并把存储格式字符串转换为内部枚举。
   */
  static RC            create(Db *db, const CreateTableSqlNode &create_table, Stmt *&stmt);

  /**
   * @brief 把 SQL 中的存储格式字符串映射为内部枚举。
   * @param format_str 语法层记录的文本格式。
   * @return 返回规范化后的存储格式。
   */
  static StorageFormat get_storage_format(const char *format_str);

private:
  string                  table_name_;      ///< 目标表名
  vector<AttrInfoSqlNode> attr_infos_;      ///< 字段定义
  vector<string>          primary_keys_;    ///< 主键列名
  StorageFormat           storage_format_;  ///< 规范化后的存储格式
};
