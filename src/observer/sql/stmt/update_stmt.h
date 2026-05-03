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
// Created by Wangyunlai on 2022/5/22.
//

#pragma once

#include "common/sys/rc.h"
#include "sql/stmt/stmt.h"

class Table;

/**
 * @file update_stmt.h
 * @brief 定义 `UPDATE` 的语义对象。
 */

/**
 * @brief 表示 `UPDATE` 语句。
 * @ingroup Statement
 * @note 当前实现仍未完成，这个类型主要保留接口边界。
 */
class UpdateStmt : public Stmt
{
public:
  UpdateStmt() = default;
  /**
   * @brief 构造更新语句。
   * @param table 目标表。
   * @param values 待更新值数组。
   * @param value_amount 待更新值个数。
   */
  UpdateStmt(Table *table, Value *values, int value_amount);

public:
  /**
   * @brief 从 parse 节点构造更新语句。
   * @return 当前实现未完成，调用方会收到错误码。
   */
  static RC create(Db *db, const UpdateSqlNode &update_sql, Stmt *&stmt);

public:
  /// 返回目标表。
  Table *table() const { return table_; }
  /// 返回待写入值数组。
  Value *values() const { return values_; }
  /// 返回待写入值个数。
  int    value_amount() const { return value_amount_; }

private:
  Table *table_        = nullptr;  ///< 目标表
  Value *values_       = nullptr;  ///< 待更新值数组
  int    value_amount_ = 0;        ///< 待更新值个数
};
