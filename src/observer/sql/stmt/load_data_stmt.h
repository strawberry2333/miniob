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
// Created by Wangyunlai on 2023/7/12.
//

#pragma once

#include "sql/stmt/stmt.h"

class Table;

/**
 * @file load_data_stmt.h
 * @brief 定义 `LOAD DATA` 的语义对象。
 */

/**
 * @brief 表示从文件导入数据的语句。
 * @details 这一层会把目标表和文件参数绑定好，执行阶段再真正逐行读取文件。
 */
class LoadDataStmt : public Stmt
{
public:
  /**
   * @brief 构造导入语句。
   * @param table 目标表。
   * @param filename 文件路径。
   * @param terminated 字段分隔符。
   * @param enclosed 文本包裹符。
   */
  LoadDataStmt(Table *table, const char *filename, const char terminated, const char enclosed)
      : table_(table), filename_(filename), terminated_(terminated), enclosed_(enclosed)
  {}
  virtual ~LoadDataStmt() = default;

  StmtType type() const override { return StmtType::LOAD_DATA; }

  /// 返回目标表。
  Table      *table() const { return table_; }
  /// 返回数据文件路径。
  const char *filename() const { return filename_.c_str(); }
  /// 返回字段分隔符。
  const char  terminated() const { return terminated_; }
  /// 返回文本包裹符。
  const char  enclosed() const { return enclosed_; }

  /**
   * @brief 从 parse 节点构造导入语句，并检查表和文件是否可用。
   */
  static RC create(Db *db, const LoadDataSqlNode &load_data, Stmt *&stmt);

private:
  Table *table_ = nullptr;  ///< 目标表
  string filename_;         ///< 文件路径
  char   terminated_;       ///< 字段分隔符
  char   enclosed_;         ///< 文本包裹符
};
