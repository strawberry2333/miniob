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
// Created by WangYunlai on 2022/11/18.
//

#include "sql/executor/sql_result.h"
#include "common/log/log.h"
#include "common/sys/rc.h"
#include "session/session.h"
#include "storage/trx/trx.h"

/**
 * @file sql_result.cpp
 * @brief 实现结果集打开、迭代和事务收尾逻辑。
 */

SqlResult::SqlResult(Session *session) : session_(session) {}

void SqlResult::set_tuple_schema(const TupleSchema &schema) { tuple_schema_ = schema; }

RC SqlResult::open()
{
  if (nullptr == operator_) {
    return RC::INVALID_ARGUMENT;
  }

  // 对自动事务场景，首次读取结果前确保事务已经就绪。
  Trx *trx = session_->current_trx();
  trx->start_if_need();
  return operator_->open(trx);
}

RC SqlResult::close()
{
  if (nullptr == operator_) {
    return RC::INVALID_ARGUMENT;
  }
  RC rc = operator_->close();
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to close operator. rc=%s", strrc(rc));
  }

  // 算子关闭后立刻释放，避免调用方继续误用旧结果集。
  operator_.reset();

  if (session_ && !session_->is_trx_multi_operation_mode()) {
    // 单语句事务模式下，由结果集在 close 阶段统一提交或回滚。
    if (rc == RC::SUCCESS) {
      rc = session_->current_trx()->commit();
    } else {
      RC rc2 = session_->current_trx()->rollback();
      if (rc2 != RC::SUCCESS) {
        LOG_PANIC("rollback failed. rc=%s", strrc(rc2));
      }
    }
    session_->destroy_trx();
  }
  return rc;
}

RC SqlResult::next_tuple(Tuple *&tuple)
{
  // `next()` 推进游标，`current_tuple()` 读取推进后的当前位置。
  RC rc = operator_->next();
  if (rc != RC::SUCCESS) {
    return rc;
  }

  tuple = operator_->current_tuple();
  return rc;
}

RC SqlResult::next_chunk(Chunk &chunk)
{
  // chunk 模式下由物理算子直接批量填充目标 chunk。
  RC rc = operator_->next(chunk);
  return rc;
}

void SqlResult::set_operator(unique_ptr<PhysicalOperator> oper)
{
  // 一个 SqlResult 生命周期内只能绑定一个未关闭的算子，避免结果互相覆盖。
  ASSERT(operator_ == nullptr, "current operator is not null. Result is not closed?");
  operator_ = std::move(oper);
  operator_->tuple_schema(tuple_schema_);
}
