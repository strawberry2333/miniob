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
// Created by Wangyunlai on 2023/6/29.
//

#pragma once

#include <string>
#include <vector>

#include "sql/stmt/stmt.h"

/**
 * @file set_variable_stmt.h
 * @brief 定义 `SET variable = value` 的语义对象。
 */

/**
 * @brief SetVairable 语句，设置变量，当前是会话变量，但是只有会话变量，没有全局变量
 * @ingroup Statement
 */
class SetVariableStmt : public Stmt
{
public:
  /**
   * @brief 以 parse 节点构造设置变量语句。
   */
  SetVariableStmt(const SetVariableSqlNode &set_variable) : set_variable_(set_variable) {}
  virtual ~SetVariableStmt() = default;

  StmtType type() const override { return StmtType::SET_VARIABLE; }

  /// 返回变量名。
  const char  *var_name() const { return set_variable_.name.c_str(); }
  /// 返回变量值。
  const Value &var_value() const { return set_variable_.value; }

  /**
   * @brief 构造设置变量语句对象。
   * @note 这一层不检查变量名是否合法，具体校验放到 executor。
   */
  static RC create(const SetVariableSqlNode &set_variable, Stmt *&stmt)
  {
    /// 可以校验是否存在某个变量，但是这里忽略
    stmt = new SetVariableStmt(set_variable);
    return RC::SUCCESS;
  }

private:
  SetVariableSqlNode set_variable_;  ///< 变量名和值
};
