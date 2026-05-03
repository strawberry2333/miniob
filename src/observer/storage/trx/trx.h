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
// Created by Wangyunlai on 2021/5/24.
//

#pragma once

#include <stddef.h>
#include <utility>

#include "common/sys/rc.h"
#include "common/lang/mutex.h"
#include "sql/parser/parse.h"
#include "storage/field/field_meta.h"
#include "storage/record/record_manager.h"
#include "storage/table/table.h"

/**
 * @defgroup Transaction
 * @brief 事务相关的内容
 */

class Db;
class LogHandler;
class LogEntry;
class Trx;
class LogReplayer;

/**
 * @file trx.h
 * @brief 定义事务抽象接口和事务管理器工厂。
 */

/**
 * @brief 描述一个操作，比如插入、删除行等
 * @ingroup Transaction
 * @details 通常包含一个操作的类型，以及操作的对象和具体的数据
 * @note 这个名称太通用，可以考虑改成更具体的名称
 */
class Operation
{
public:
  /**
   * @brief 操作的类型
   * @ingroup Transaction
   */
  enum class Type : int
  {
    INSERT,
    UPDATE,
    DELETE,
    UNDEFINED,
  };

public:
  Operation(Type type, Table *table, const RID &rid)
      : type_(type), table_(table), page_num_(rid.page_num), slot_num_(rid.slot_num)
  {}

  Type    type() const { return type_; }
  int32_t table_id() const { return table_->table_id(); }
  Table  *table() const { return table_; }
  PageNum page_num() const { return page_num_; }
  SlotNum slot_num() const { return slot_num_; }

private:
  ///< 操作的哪张表。这里直接使用表其实并不准确，因为表中的索引也可能有日志
  Type type_;

  Table  *table_ = nullptr;
  PageNum page_num_;  // TODO use RID instead of page num and slot num
  SlotNum slot_num_;
};

class OperationHasher
{
public:
  size_t operator()(const Operation &op) const { return (((size_t)op.page_num()) << 32) | (op.slot_num()); }
};

class OperationEqualer
{
public:
  bool operator()(const Operation &op1, const Operation &op2) const
  {
    return op1.table_id() == op2.table_id() && op1.page_num() == op2.page_num() && op1.slot_num() == op2.slot_num();
  }
};

/**
 * @brief 事务管理器
 * @ingroup Transaction
 */
class TrxKit
{
public:
  /**
   * @brief 事务管理器的类型
   * @ingroup Transaction
   * @details 进程启动时根据事务管理器的类型来创建具体的对象
   */
  enum Type
  {
    VACUOUS,  ///< 空的事务管理器，不做任何事情
    MVCC,     ///< 支持MVCC的事务管理器
    LSM,      ///< 支持LSM的事务管理器
  };

public:
  TrxKit()          = default;
  virtual ~TrxKit() = default;

  virtual RC                       init()             = 0;
  /// @brief 返回所有表都必须额外挂载的事务隐藏字段定义。
  virtual const vector<FieldMeta> *trx_fields() const = 0;

  /// @brief 创建一个全新的运行时事务对象。
  virtual Trx *create_trx(LogHandler &log_handler) = 0;

  /**
   * @brief 创建一个事务，日志回放时使用
   */
  virtual Trx *create_trx(LogHandler &log_handler, int32_t trx_id) = 0;
  virtual void all_trxes(vector<Trx *> &trxes)                     = 0;

  virtual void destroy_trx(Trx *trx) = 0;

  /// @brief 创建与当前事务模型匹配的日志回放器。
  virtual LogReplayer *create_log_replayer(Db &db, LogHandler &log_handler) = 0;

public:
static TrxKit *create(const char *name, Db *db);
};

/**
 * @brief 事务接口
 * @ingroup Transaction
 */
class Trx
{
public:
  Trx(TrxKit::Type type) : type_(type) {}
  virtual ~Trx() = default;

  /// @brief 在事务上下文中插入记录。
  virtual RC insert_record(Table *table, Record &record)                         = 0;
  /// @brief 在事务上下文中删除记录。
  virtual RC delete_record(Table *table, Record &record)                         = 0;
  /// @brief 在事务上下文中更新记录。
  virtual RC update_record(Table *table, Record &old_record, Record &new_record) = 0;
  /// @brief 探测一条记录对当前事务是否可见、是否允许修改。
  virtual RC visit_record(Table *table, Record &record, ReadWriteMode mode)      = 0;

  /// @brief 惰性启动事务。
  virtual RC start_if_need() = 0;
  /// @brief 提交事务。
  virtual RC commit()        = 0;
  /// @brief 回滚事务。
  virtual RC rollback()      = 0;

  /// @brief 根据持久化日志做重放。
  virtual RC redo(Db *db, const LogEntry &log_entry) = 0;

  virtual int32_t id() const = 0;
  TrxKit::Type    type() const { return type_; }

private:
  TrxKit::Type type_;
};
