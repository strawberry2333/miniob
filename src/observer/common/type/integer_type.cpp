/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "common/lang/comparator.h"
#include "common/lang/sstream.h"
#include "common/log/log.h"
#include "common/type/integer_type.h"
#include "common/value.h"
#include "storage/common/column.h"

/**
 * @brief IntegerType 的实现。
 * @details 这里统一使用整数语义处理算术运算，并在需要时退化为浮点比较。
 */

/**
 * @brief 比较一个整数值与另一个数值。
 * @param left 左操作数，必须是 INTS。
 * @param right 右操作数，可以是 INTS 或 FLOATS。
 * @return 标准三路比较结果。
 */
int IntegerType::compare(const Value &left, const Value &right) const
{
  ASSERT(left.attr_type() == AttrType::INTS, "left type is not integer");
  ASSERT(right.attr_type() == AttrType::INTS || right.attr_type() == AttrType::FLOATS, "right type is not numeric");
  if (right.attr_type() == AttrType::INTS) {
    // 两边都是整数时直接按整数位模式比较，避免不必要的浮点转换。
    return common::compare_int((void *)&left.value_.int_value_, (void *)&right.value_.int_value_);
  } else if (right.attr_type() == AttrType::FLOATS) {
    // 混合比较时统一提升为 float，保证比较语义与算术运算一致。
    float left_val  = left.get_float();
    float right_val = right.get_float();
    return common::compare_float((void *)&left_val, (void *)&right_val);
  }
  return INT32_MAX;
}

/**
 * @brief 比较列式存储中的两个整数元素。
 * @param left 左列。
 * @param right 右列。
 * @param left_idx 左列下标。
 * @param right_idx 右列下标。
 * @return 标准三路比较结果。
 */
int IntegerType::compare(const Column &left, const Column &right, int left_idx, int right_idx) const
{
  ASSERT(left.attr_type() == AttrType::INTS, "left type is not integer");
  ASSERT(right.attr_type() == AttrType::INTS, "right type is not integer");
  return common::compare_int((void *)&((int*)left.data())[left_idx],
      (void *)&((int*)right.data())[right_idx]);
}

/**
 * @brief 把整数转换到目标类型。
 * @param val 源整数值。
 * @param type 目标类型。
 * @param result 输出结果。
 * @return 支持转成 FLOATS，其余返回类型不匹配错误。
 */
RC IntegerType::cast_to(const Value &val, AttrType type, Value &result) const
{
  switch (type) {
  case AttrType::FLOATS: {
    float float_value = val.get_int();
    result.set_float(float_value);
    return RC::SUCCESS;
  }
  default:
    LOG_WARN("unsupported type %d", type);
    return RC::SCHEMA_FIELD_TYPE_MISMATCH;
  }
}

/// @brief 执行整数加法，并把结果写入 result。
RC IntegerType::add(const Value &left, const Value &right, Value &result) const
{
  result.set_int(left.get_int() + right.get_int());
  return RC::SUCCESS;
}

/// @brief 执行整数减法，并把结果写入 result。
RC IntegerType::subtract(const Value &left, const Value &right, Value &result) const
{
  result.set_int(left.get_int() - right.get_int());
  return RC::SUCCESS;
}

/// @brief 执行整数乘法，并把结果写入 result。
RC IntegerType::multiply(const Value &left, const Value &right, Value &result) const
{
  result.set_int(left.get_int() * right.get_int());
  return RC::SUCCESS;
}

/// @brief 计算整数值的一元负号。
RC IntegerType::negative(const Value &val, Value &result) const
{
  result.set_int(-val.get_int());
  return RC::SUCCESS;
}

/**
 * @brief 从文本反序列化整数 Value。
 * @param val 输出 Value。
 * @param data 输入文本。
 * @return 成功返回 SUCCESS，格式不合法返回 SCHEMA_FIELD_TYPE_MISMATCH。
 */
RC IntegerType::set_value_from_str(Value &val, const string &data) const
{
  RC                rc = RC::SUCCESS;
  stringstream deserialize_stream;
  deserialize_stream.clear();  // 清理stream的状态，防止多次解析出现异常
  deserialize_stream.str(data);
  int int_value;
  deserialize_stream >> int_value;
  if (!deserialize_stream || !deserialize_stream.eof()) {
    rc = RC::SCHEMA_FIELD_TYPE_MISMATCH;
  } else {
    val.set_int(int_value);
  }
  return rc;
}

/**
 * @brief 把整数 Value 序列化为字符串。
 * @param val 源值。
 * @param result 输出字符串。
 * @return 成功状态码。
 */
RC IntegerType::to_string(const Value &val, string &result) const
{
  stringstream ss;
  ss << val.value_.int_value_;
  result = ss.str();
  return RC::SUCCESS;
}
