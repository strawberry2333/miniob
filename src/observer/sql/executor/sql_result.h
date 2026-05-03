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
// Created by WangYunlai on 2022/11/17.
//

#pragma once

#include "common/lang/string.h"
#include "common/lang/memory.h"
#include "sql/expr/tuple.h"
#include "sql/operator/physical_operator.h"

class Session;

/**
 * @file sql_result.h
 * @brief 定义 SQL 执行结果对象及其事务收尾职责。
 */

/**
 * @brief SQL执行结果
 * @details
 * 如果当前SQL生成了执行计划，那么在返回客户端时，调用执行计划返回结果。
 * 否则返回的结果就是当前SQL的执行结果，比如DDL语句，通过return_code和state_string来描述。
 * 如果出现了一些错误，也可以通过return_code和state_string来获取信息。
 */
class SqlResult
{
public:
  /**
   * @brief 构造一个绑定到会话的 SQL 返回结果对象。
   * @param session 结果所属会话，用于驱动事务开关和自动提交。
   */
  SqlResult(Session *session);
  ~SqlResult() {}

  /// 设置结果集的输出 schema，供后续算子或客户端读取。
  void set_tuple_schema(const TupleSchema &schema);
  /// 设置命令类 SQL 的返回码。
  void set_return_code(RC rc) { return_code_ = rc; }
  /// 设置用户可读的状态文本。
  void set_state_string(const string &state_string) { state_string_ = state_string; }

  /**
   * @brief 接管物理算子所有权，并把当前 schema 下发给算子。
   * @param oper 物理算子树根节点。
   */
  void set_operator(unique_ptr<PhysicalOperator> oper);

  bool               has_operator() const { return operator_ != nullptr; }
  const TupleSchema &tuple_schema() const { return tuple_schema_; }
  RC                 return_code() const { return return_code_; }
  const string      &state_string() const { return state_string_; }

  /**
   * @brief 打开结果集对应的物理算子。
   * @return 返回算子 `open` 的结果。
   */
  RC open();

  /**
   * @brief 关闭结果集并处理自动事务提交/回滚。
   * @return 返回关闭或事务收尾结果。
   */
  RC close();

  /**
   * @brief 拉取下一条 tuple 结果。
   * @param tuple 输出当前 tuple 指针。
   * @return 返回迭代状态。
   */
  RC next_tuple(Tuple *&tuple);

  /**
   * @brief 拉取下一批 chunk 结果。
   * @param chunk 输出 chunk。
   * @return 返回迭代状态。
   */
  RC next_chunk(Chunk &chunk);

private:
  Session                     *session_ = nullptr;  ///< 当前所属会话
  unique_ptr<PhysicalOperator> operator_;           ///< 当前结果集绑定的物理算子树
  TupleSchema                  tuple_schema_;       ///< 返回表头信息，命令类语句可以为空
  RC                           return_code_ = RC::SUCCESS;
  string                       state_string_;       ///< 用户可读状态文本
};
