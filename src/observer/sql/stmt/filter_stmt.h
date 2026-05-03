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
// Created by Wangyunlai on 2022/5/22.
//

#pragma once

#include "common/lang/unordered_map.h"
#include "common/lang/vector.h"
#include "sql/expr/expression.h"
#include "sql/parser/parse_defs.h"
#include "sql/stmt/stmt.h"

class Db;
class Table;
class FieldMeta;

/**
 * @file filter_stmt.h
 * @brief 定义 where 条件绑定后的过滤结构。
 */

/**
 * @brief 表示过滤条件一侧的操作数。
 * @details 一侧操作数要么是具体字段，要么是常量值；执行阶段根据 `is_attr`
 * 判断如何从 tuple 中取值或直接使用常量。
 */
struct FilterObj
{
  bool  is_attr;  ///< `true` 表示当前对象引用字段，`false` 表示常量值
  Field field;    ///< 字段引用
  Value value;    ///< 常量值

  /**
   * @brief 把当前操作数初始化为字段引用。
   * @param field 已绑定字段。
   */
  void init_attr(const Field &field)
  {
    is_attr     = true;
    this->field = field;
  }

  /**
   * @brief 把当前操作数初始化为常量值。
   * @param value SQL 字面量。
   */
  void init_value(const Value &value)
  {
    is_attr     = false;
    this->value = value;
  }
};

/**
 * @brief 表示一个完成字段绑定的比较条件。
 * @details 当前一个 `FilterUnit` 对应 `where` 中的一个二元比较表达式。
 */
class FilterUnit
{
public:
  FilterUnit() = default;
  ~FilterUnit() {}

  /// 设置比较运算符。
  void set_comp(CompOp comp) { comp_ = comp; }

  /// 返回比较运算符。
  CompOp comp() const { return comp_; }

  /// 设置左侧操作数。
  void set_left(const FilterObj &obj) { left_ = obj; }
  /// 设置右侧操作数。
  void set_right(const FilterObj &obj) { right_ = obj; }

  /// 返回左侧操作数。
  const FilterObj &left() const { return left_; }
  /// 返回右侧操作数。
  const FilterObj &right() const { return right_; }

private:
  CompOp    comp_ = NO_OP;  ///< 比较运算符
  FilterObj left_;          ///< 左侧操作数
  FilterObj right_;         ///< 右侧操作数
};

/**
 * @brief 表示一个 `WHERE` 子句绑定后的过滤描述。
 * @ingroup Statement
 * @details 当前所有过滤单元默认按 AND 关系拼接，尚未表达 OR/NOT 等更复杂逻辑。
 */
class FilterStmt
{
public:
  FilterStmt() = default;
  virtual ~FilterStmt();

public:
  /// 返回所有过滤单元。
  const vector<FilterUnit *> &filter_units() const { return filter_units_; }

public:
  /**
   * @brief 批量把 parse 阶段的条件列表绑定成 `FilterStmt`。
   * @param db 当前数据库。
   * @param default_table 单表语句时的默认表。
   * @param tables 当前语句可见的表映射。
   * @param conditions parse 阶段产出的条件数组。
   * @param condition_num 条件个数。
   * @param stmt 输出过滤语句对象。
   * @return 返回绑定结果。
   */
  static RC create(Db *db, Table *default_table, unordered_map<string, Table *> *tables,
      const ConditionSqlNode *conditions, int condition_num, FilterStmt *&stmt);

  /**
   * @brief 把单个 parse 条件绑定成 `FilterUnit`。
   * @param db 当前数据库。
   * @param default_table 单表语句时的默认表。
   * @param tables 当前语句可见的表映射。
   * @param condition parse 阶段条件节点。
   * @param filter_unit 输出过滤单元。
   * @return 返回绑定结果。
   */
  static RC create_filter_unit(Db *db, Table *default_table, unordered_map<string, Table *> *tables,
      const ConditionSqlNode &condition, FilterUnit *&filter_unit);

private:
  vector<FilterUnit *> filter_units_;  ///< 默认当前所有单元都按 AND 关系组合
};
