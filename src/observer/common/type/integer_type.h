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

#include "common/type/data_type.h"

/**
 * @brief 整数类型处理器定义。
 * @details 负责 INTS 类型的比较、整数算术、向浮点的类型转换以及文本反序列化。
 */

/**
 * @brief 整型类型
 * @ingroup DataType
 */
class IntegerType : public DataType
{
public:
  IntegerType() : DataType(AttrType::INTS) {}
  virtual ~IntegerType() {}

  /// @brief 比较一个整数 Value 与另一个数值 Value。
  int compare(const Value &left, const Value &right) const override;
  /// @brief 比较列式存储中的两个整型值。
  int compare(const Column &left, const Column &right, int left_idx, int right_idx) const override;

  /// @brief 执行整数加法。
  RC add(const Value &left, const Value &right, Value &result) const override;
  /// @brief 执行整数减法。
  RC subtract(const Value &left, const Value &right, Value &result) const override;
  /// @brief 执行整数乘法。
  RC multiply(const Value &left, const Value &right, Value &result) const override;
  /// @brief 计算一元负号。
  RC negative(const Value &val, Value &result) const override;

  /// @brief 把整数转换到目标类型；当前只支持转成浮点。
  RC cast_to(const Value &val, AttrType type, Value &result) const override;

  /// @brief 返回从整数隐式转换到目标类型的代价。
  int cast_cost(const AttrType type) override
  {
    if (type == AttrType::INTS) {
      return 0;
    } else if (type == AttrType::FLOATS) {
      return 1;
    }
    return INT32_MAX;
  }

  /// @brief 从文本解析一个整数 Value。
  RC set_value_from_str(Value &val, const string &data) const override;

  /// @brief 把整数 Value 转成字符串。
  RC to_string(const Value &val, string &result) const override;
};
