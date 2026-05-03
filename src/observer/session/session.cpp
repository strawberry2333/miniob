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

#include "session/session.h"
#include "common/global_context.h"
#include "storage/db/db.h"
#include "storage/default/default_handler.h"
#include "storage/trx/trx.h"

/**
 * @brief Session 的实现文件。
 * @details 这里同时处理连接级共享状态，以及请求线程通过 thread_local 访问的会话上下文。
 */

/**
 * @brief 返回默认会话模板。
 * @return 进程级唯一的默认 Session。
 */
Session &Session::default_session()
{
  static Session session;
  return session;
}

/// @brief 拷贝可共享的数据库绑定状态，不复制事务与请求上下文。
Session::Session(const Session &other) : db_(other.db_) {}

/**
 * @brief 析构会话。
 * @details 如果会话仍持有事务对象，则通过所属数据库的 trx_kit 安全释放。
 */
Session::~Session()
{
  if (nullptr != trx_) {
    db_->trx_kit().destroy_trx(trx_);
    trx_ = nullptr;
  }
}

/**
 * @brief 返回当前数据库名。
 * @return 若已绑定数据库则返回数据库名，否则返回空串。
 */
const char *Session::get_current_db_name() const
{
  if (db_ != nullptr)
    return db_->name();
  else
    return "";
}

Db *Session::get_current_db() const { return db_; }

/**
 * @brief 切换当前会话绑定的数据库。
 * @param dbname 目标数据库名。
 * @note 如果数据库不存在，则保持原有 db_ 不变。
 */
void Session::set_current_db(const string &dbname)
{
  DefaultHandler &handler = *GCTX.handler_;
  Db             *db      = handler.find_db(dbname.c_str());
  if (db == nullptr) {
    LOG_WARN("no such database: %s", dbname.c_str());
    return;
  }

  LOG_TRACE("change db to %s", dbname.c_str());
  db_ = db;
}

/// @brief 设置事务是否进入多语句模式。
void Session::set_trx_multi_operation_mode(bool multi_operation_mode)
{
  trx_multi_operation_mode_ = multi_operation_mode;
}

/// @brief 返回当前事务是否处于多语句模式。
bool Session::is_trx_multi_operation_mode() const { return trx_multi_operation_mode_; }

/**
 * @brief 获取当前会话对应的事务对象。
 * @return 当前事务；若此前尚未创建则会按需创建并缓存。
 */
Trx *Session::current_trx()
{
  /*
  当前把事务与数据库绑定到了一起。这样虽然不合理，但是处理起来也简单。
  我们在测试过程中，也不需要多个数据库之间做关联。
  */
  if (trx_ == nullptr) {
    // 首次访问事务时才真正向数据库申请，避免无事务语句产生额外开销。
    trx_ = db_->trx_kit().create_trx(db_->log_handler());
  }
  return trx_;
}

/**
 * @brief 主动销毁当前事务对象。
 * @details 适用于 commit/rollback 完成之后重置会话事务状态。
 */
void Session::destroy_trx()
{
  if (trx_ != nullptr) {
    db_->trx_kit().destroy_trx(trx_);
    trx_ = nullptr;
  }
}

/// @brief 每个工作线程持有自己当前正在服务的 Session 指针。
thread_local Session *thread_session = nullptr;

/// @brief 把当前线程绑定到某个 Session，请求结束时应显式清空。
void Session::set_current_session(Session *session) { thread_session = session; }

/// @brief 返回当前线程正在处理的 Session；非请求线程可能返回空。
Session *Session::current_session() { return thread_session; }

/// @brief 记录当前会话正在处理的请求对象。
void Session::set_current_request(SessionEvent *request) { current_request_ = request; }

/// @brief 返回当前会话正在处理的请求对象。
SessionEvent *Session::current_request() const { return current_request_; }
