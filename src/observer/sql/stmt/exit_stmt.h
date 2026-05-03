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

#include "sql/stmt/stmt.h"

/**
 * @file exit_stmt.h
 * @brief 定义 `EXIT` 语句的语义对象。
 */

/**
 * @brief 表示客户端请求断开连接的语句。
 * @ingroup Statement
 */
class ExitStmt : public Stmt
{
public:
  ExitStmt() {}
  virtual ~ExitStmt() = default;

  StmtType type() const override { return StmtType::EXIT; }

  /**
   * @brief 创建一个无状态的 `ExitStmt`。
   * @param stmt 输出语句对象。
   * @return 总是返回 `RC::SUCCESS`。
   */
  static RC create(Stmt *&stmt)
  {
    stmt = new ExitStmt();
    return RC::SUCCESS;
  }
};
