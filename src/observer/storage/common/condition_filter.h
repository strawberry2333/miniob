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
// Created by Wangyunlai on 2021/5/7.
//

#pragma once

#include "sql/parser/parse.h"

/**
 * @file condition_filter.h
 * @brief 定义记录级谓词过滤器，用于扫描阶段做行裁剪。
 */

class Record;
class Table;

/**
 * @brief 条件一侧操作数的绑定结果。
 * @details 解析 SQL 条件后，会把“字段引用”或“常量值”统一整理为该结构，
 * 供过滤器在运行时快速从 Record 中取值并比较。
 */
struct ConDesc
{
  bool  is_attr;      ///< 是否引用记录字段；`false` 表示直接使用常量值。
  int   attr_length;  ///< 字段值长度，仅 `is_attr=true` 时有效。
  int   attr_offset;  ///< 字段在记录中的偏移，仅 `is_attr=true` 时有效。
  Value value;        ///< 常量值描述，仅 `is_attr=false` 时有效。
};

/**
 * @brief 所有记录过滤器的抽象基类。
 * @details 过滤器本身不负责并发控制；调用方必须保证传入 Record 指向的数据在判定期间有效。
 */
class ConditionFilter
{
public:
  virtual ~ConditionFilter();

  /**
   * @brief 判断一条记录是否满足条件。
   * @return `true` 表示命中条件，`false` 表示应被扫描器跳过。
   */
  virtual bool filter(const Record &rec) const = 0;
};

/**
 * @brief 处理单个比较谓词的过滤器。
 * @details 负责把左右操作数绑定到具体字段偏移或常量值，并在运行时执行一次比较。
 */
class DefaultConditionFilter : public ConditionFilter
{
public:
  DefaultConditionFilter();
  virtual ~DefaultConditionFilter();

  /**
   * @brief 直接用已解析好的左右操作数初始化过滤器。
   * @details 该接口只做类型和比较符合法性校验，不解析字段名。
   */
  RC init(const ConDesc &left, const ConDesc &right, AttrType attr_type, CompOp comp_op);
  /**
   * @brief 从表元数据和 SQL 条件解析出可执行过滤器。
   * @details 字段不存在、类型不兼容等错误会在这里尽早返回。
   */
  RC init(Table &table, const ConditionSqlNode &condition);

  virtual bool filter(const Record &rec) const;

public:
  const ConDesc &left() const { return left_; }
  const ConDesc &right() const { return right_; }

  CompOp   comp_op() const { return comp_op_; }
  AttrType attr_type() const { return attr_type_; }

private:
  ConDesc  left_;
  ConDesc  right_;
  AttrType attr_type_ = AttrType::UNDEFINED;
  CompOp   comp_op_   = NO_OP;
};

/**
 * @brief 将多个过滤器按 AND 关系组合。
 * @details `CompositeConditionFilter` 只负责顺序执行子过滤器。
 * 如果 `memory_owner_` 为真，则析构时还会释放 `filters_` 数组和其中的子过滤器对象。
 */
class CompositeConditionFilter : public ConditionFilter
{
public:
  CompositeConditionFilter() = default;
  virtual ~CompositeConditionFilter();

  /**
   * @brief 直接绑定一组现成的子过滤器。
   * @details 该重载不会接管子过滤器所有权，调用方负责其生命周期。
   */
  RC init(const ConditionFilter *filters[], int filter_num);
  /**
   * @brief 从 SQL 条件数组批量构造子过滤器并接管其所有权。
   */
  RC init(Table &table, const ConditionSqlNode *conditions, int condition_num);

  virtual bool filter(const Record &rec) const;

public:
  int                    filter_num() const { return filter_num_; }
  const ConditionFilter &filter(int index) const { return *filters_[index]; }

private:
  /**
   * @brief 设置内部过滤器数组及所有权策略。
   */
  RC init(const ConditionFilter *filters[], int filter_num, bool own_memory);

private:
  const ConditionFilter **filters_      = nullptr;
  int                     filter_num_   = 0;
  bool                    memory_owner_ = false;  // filters_的内存是否由自己来控制
};
