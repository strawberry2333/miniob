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
 * @file trx_end_stmt.h
 * @brief 定义 `COMMIT` / `ROLLBACK` 的语义对象。
 */

/**
 * @brief 表示显式事务结束语句。
 * @ingroup Statement
 */
class TrxEndStmt : public Stmt
{
public:
  /**
   * @brief 按具体结束类型构造事务语句。
   * @param type `COMMIT` 或 `ROLLBACK`。
   */
  TrxEndStmt(StmtType type) : type_(type) {}
  virtual ~TrxEndStmt() = default;

  StmtType type() const override { return type_; }

  /**
   * @brief 根据 parse 阶段命令标记创建提交或回滚语句。
   */
  static RC create(SqlCommandFlag flag, Stmt *&stmt)
  {
    StmtType type = flag == SqlCommandFlag::SCF_COMMIT ? StmtType::COMMIT : StmtType::ROLLBACK;
    stmt          = new TrxEndStmt(type);
    return RC::SUCCESS;
  }

private:
  StmtType type_;  ///< 实际事务结束类型
};
