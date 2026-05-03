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

#pragma once

#include "common/types.h"
#include "common/lang/string.h"

class Trx;
class Db;
class SessionEvent;

/**
 * @brief observer 会话对象定义。
 * @details 一个连接对应一个 Session，用来承载数据库绑定、事务状态以及会话级功能开关。
 */

/**
 * @brief 表示会话
 * @details 当前一个连接一个会话，没有做特殊的会话管理，这也简化了会话处理
 */
class Session
{
public:
  /**
   * @brief 获取默认的会话数据，新生成的会话都基于默认会话设置参数
   * @note 当前并没有会话参数
   */
  static Session &default_session();

public:
  Session() = default;
  /// @brief 析构会话时回收当前尚未释放的事务对象。
  ~Session();

  /// @brief 复制会话时只复制可共享配置，不复制事务和当前请求。
  Session(const Session &other);
  void operator=(Session &) = delete;

  /// @brief 返回当前绑定数据库的名字；未绑定数据库时返回空串。
  const char *get_current_db_name() const;
  /// @brief 返回当前绑定的数据库对象，可能为空。
  Db         *get_current_db() const;

  /**
   * @brief 设置当前会话关联的数据库
   *
   * @param dbname 数据库名字
   */
  void set_current_db(const string &dbname);

  /**
   * @brief 设置当前事务为多语句模式，需要明确的指出提交或回滚
   */
  void set_trx_multi_operation_mode(bool multi_operation_mode);

  /**
   * @brief 当前事务是否为多语句模式
   */
  bool is_trx_multi_operation_mode() const;

  /**
   * @brief 当前会话关联的事务
   *
   * @return 当前事务对象；若尚未创建则按需创建。
   */
  Trx *current_trx();

  /// @brief 销毁当前会话持有的事务对象，并清空指针。
  void destroy_trx();

  /**
   * @brief 设置当前正在处理的请求
   */
  void set_current_request(SessionEvent *request);

  /**
   * @brief 获取当前正在处理的请求
   */
  SessionEvent *current_request() const;

  /// @brief 打开或关闭当前会话的 SQL 调试输出。
  void set_sql_debug(bool sql_debug) { sql_debug_ = sql_debug; }
  /// @brief 返回当前会话是否开启 SQL 调试输出。
  bool sql_debug_on() const { return sql_debug_; }

  /// @brief 控制优化/执行阶段是否允许使用 hash join。
  void set_hash_join(bool hash_join) { hash_join_ = hash_join; }
  /// @brief 返回当前会话是否启用了 hash join。
  bool hash_join_on() const { return hash_join_; }

  /// @brief 控制是否使用 cascade 优化器。
  void set_use_cascade(bool use_cascade) { use_cascade_ = use_cascade; }
  /// @brief 返回当前会话是否启用了 cascade 优化器。
  bool use_cascade() const { return use_cascade_; }

  /// @brief 设置执行模式，例如 tuple iterator 或 chunk iterator。
  void          set_execution_mode(const ExecutionMode mode) { execution_mode_ = mode; }
  /// @brief 返回当前执行模式。
  ExecutionMode get_execution_mode() const { return execution_mode_; }

  /// @brief 返回当前语句最终是否真正落到了 chunk 模式。
  bool used_chunk_mode() { return used_chunk_mode_; }

  /// @brief 记录当前语句是否已经实际使用了 chunk 模式。
  void set_used_chunk_mode(bool used_chunk_mode) { used_chunk_mode_ = used_chunk_mode; }

  /**
   * @brief 将指定会话设置到线程变量中
   *
   */
  static void set_current_session(Session *session);

  /**
   * @brief 获取当前的会话
   * @details 当前某个请求开始时，会将会话设置到线程变量中，在整个请求处理过程中不会改变
   */
  static Session *current_session();

private:
  Db           *db_              = nullptr;  ///< 当前会话绑定的数据库，不拥有其生命周期
  Trx          *trx_             = nullptr;  ///< 懒创建的事务对象，由数据库的 trx_kit 管理
  SessionEvent *current_request_ = nullptr;  ///< 当前正在处理的请求

  bool trx_multi_operation_mode_ = false;  ///< 当前事务的模式，是否多语句模式. 单语句模式自动提交

  bool sql_debug_   = false;  ///< 是否输出SQL调试信息
  bool hash_join_   = false;  ///< 是否使用hash join
  bool use_cascade_ = false;  ///< 是否使用 cascade 优化器

  // 是否使用了 `chunk_iterator` 模式。 只有在设置了 `chunk_iterator`
  // 并且可以生成相关物理执行计划时才会使用 `chunk_iterator` 模式。
  bool used_chunk_mode_ = false;

  ExecutionMode execution_mode_ = ExecutionMode::TUPLE_ITERATOR;  ///< 当前会话偏好的执行模式
};
