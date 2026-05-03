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
 * @brief 浮点类型处理器定义。
 * @details 负责 FLOATS 类型的比较、算术运算、字符串序列化和文本反序列化。
 */

/**
 * @brief 浮点型数据类型
 * @ingroup DataType
 */
class FloatType : public DataType
{
public:
  FloatType() : DataType(AttrType::FLOATS) {}
  virtual ~FloatType() = default;

  /// @brief 比较一个 FLOATS Value 与另一个数值 Value。
  int compare(const Value &left, const Value &right) const override;
  /// @brief 比较列式存储中的两个浮点值。
  int compare(const Column &left, const Column &right, int left_idx, int right_idx) const override;

  /// @brief 执行浮点加法。
  RC add(const Value &left, const Value &right, Value &result) const override;
  /// @brief 执行浮点减法。
  RC subtract(const Value &left, const Value &right, Value &result) const override;
  /// @brief 执行浮点乘法。
  RC multiply(const Value &left, const Value &right, Value &result) const override;
  /// @brief 执行浮点除法。
  RC divide(const Value &left, const Value &right, Value &result) const override;
  /// @brief 计算一元负号。
  RC negative(const Value &val, Value &result) const override;

  /// @brief 从文本解析一个浮点 Value。
  RC set_value_from_str(Value &val, const string &data) const override;

  /// @brief 把浮点 Value 转成字符串。
  RC to_string(const Value &val, string &result) const override;
};
