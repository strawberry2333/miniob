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

#include "common/sys/rc.h"
#include "common/type/data_type.h"

/**
 * @brief 字符串类型处理器定义。
 * @details 负责 CHARS 类型的比较、字符串化和从文本构造 Value。
 */

/**
 * @brief 固定长度的字符串类型
 * @ingroup DataType
 */
class CharType : public DataType
{
public:
  CharType() : DataType(AttrType::CHARS) {}

  virtual ~CharType() = default;

  /// @brief 按字符串字节序比较两个 CHARS 值。
  int compare(const Value &left, const Value &right) const override;

  /// @brief 进行类型转换；当前只保留未实现分支。
  RC cast_to(const Value &val, AttrType type, Value &result) const override;

  /// @brief 从普通字符串构造一个 CHARS Value。
  RC set_value_from_str(Value &val, const string &data) const override;

  /// @brief 计算从当前类型到目标类型的隐式转换代价。
  int cast_cost(AttrType type) override;

  /// @brief 把 CHARS Value 还原成 string。
  RC to_string(const Value &val, string &result) const override;
};
