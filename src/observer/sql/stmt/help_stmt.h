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
// Created by Wangyunlai on 2023/6/14.
//

#pragma once

#include <string>
#include <vector>

#include "sql/stmt/stmt.h"

/**
 * @file help_stmt.h
 * @brief 定义 `HELP` 语句的语义对象。
 */

/**
 * @brief 表示内置帮助命令。
 * @ingroup Statement
 */
class HelpStmt : public Stmt
{
public:
  HelpStmt() {}
  virtual ~HelpStmt() = default;

  StmtType type() const override { return StmtType::HELP; }

  /**
   * @brief 创建一个无状态的帮助语句对象。
   */
  static RC create(Stmt *&stmt)
  {
    stmt = new HelpStmt();
    return RC::SUCCESS;
  }
};
