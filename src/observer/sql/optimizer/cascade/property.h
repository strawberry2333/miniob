/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#pragma once

/**
 * @file property.h
 * @brief 级联优化器中的属性类型定义。
 */

class Property
{};

/**
 * @brief Logical Property, such as the cardinality of logical operator
 */
class LogicalProperty
{
public:
  explicit LogicalProperty(int card) : card_(card) {}
  LogicalProperty()  = default;
  ~LogicalProperty() = default;

  /// @brief 返回当前逻辑表达式估计的输出基数。
  int get_card() const { return card_; }

private:
  int card_ = 0;  /// cardinality
};
