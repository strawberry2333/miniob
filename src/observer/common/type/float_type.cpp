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
#include "common/type/float_type.h"
#include "common/value.h"
#include "common/lang/limits.h"
#include "common/value.h"
#include "storage/common/column.h"

/**
 * @brief FloatType 的实现。
 * @details 这里统一把参与运算的输入先转成 float，再执行比较和算术逻辑。
 */

/**
 * @brief 比较一个浮点值与另一个数值。
 * @param left 左操作数，必须是 FLOATS。
 * @param right 右操作数，可以是 INTS 或 FLOATS。
 * @return 标准三路比较结果。
 */
int FloatType::compare(const Value &left, const Value &right) const
{
  ASSERT(left.attr_type() == AttrType::FLOATS, "left type is not float");
  ASSERT(right.attr_type() == AttrType::INTS || right.attr_type() == AttrType::FLOATS, "right type is not numeric");
  float left_val  = left.get_float();
  float right_val = right.get_float();
  return common::compare_float((void *)&left_val, (void *)&right_val);
}

/**
 * @brief 比较列式存储中的两个浮点元素。
 * @param left 左列。
 * @param right 右列。
 * @param left_idx 左列下标。
 * @param right_idx 右列下标。
 * @return 标准三路比较结果。
 */
int FloatType::compare(const Column &left, const Column &right, int left_idx, int right_idx) const
{
  ASSERT(left.attr_type() == AttrType::FLOATS, "left type is not float");
  ASSERT(right.attr_type() == AttrType::FLOATS, "right type is not float");
  return common::compare_float((void *)&((float*)left.data())[left_idx],
      (void *)&((float*)right.data())[right_idx]);
}

/// @brief 执行浮点加法，并把结果写入 result。
RC FloatType::add(const Value &left, const Value &right, Value &result) const
{
  result.set_float(left.get_float() + right.get_float());
  return RC::SUCCESS;
}
/// @brief 执行浮点减法，并把结果写入 result。
RC FloatType::subtract(const Value &left, const Value &right, Value &result) const
{
  result.set_float(left.get_float() - right.get_float());
  return RC::SUCCESS;
}
/// @brief 执行浮点乘法，并把结果写入 result。
RC FloatType::multiply(const Value &left, const Value &right, Value &result) const
{
  result.set_float(left.get_float() * right.get_float());
  return RC::SUCCESS;
}

/**
 * @brief 执行浮点除法。
 * @param left 被除数。
 * @param right 除数。
 * @param result 结果输出。
 * @note 当前系统没有 NULL 语义，因此除零时退化为设置成 float 最大值。
 */
RC FloatType::divide(const Value &left, const Value &right, Value &result) const
{
  if (right.get_float() > -EPSILON && right.get_float() < EPSILON) {
    // NOTE:
    // 设置为浮点数最大值是不正确的。通常的做法是设置为NULL，但是当前的miniob没有NULL概念，所以这里设置为浮点数最大值。
    result.set_float(numeric_limits<float>::max());
  } else {
    result.set_float(left.get_float() / right.get_float());
  }
  return RC::SUCCESS;
}

/// @brief 计算浮点值的一元负号。
RC FloatType::negative(const Value &val, Value &result) const
{
  result.set_float(-val.get_float());
  return RC::SUCCESS;
}

/**
 * @brief 从文本反序列化浮点 Value。
 * @param val 输出 Value。
 * @param data 输入文本。
 * @return 成功返回 SUCCESS，格式不合法返回 SCHEMA_FIELD_TYPE_MISMATCH。
 */
RC FloatType::set_value_from_str(Value &val, const string &data) const
{
  RC                rc = RC::SUCCESS;
  stringstream deserialize_stream;
  // stringstream 复用前先重置状态，避免残留错误位影响本次解析。
  deserialize_stream.clear();
  deserialize_stream.str(data);

  float float_value;
  deserialize_stream >> float_value;
  if (!deserialize_stream || !deserialize_stream.eof()) {
    rc = RC::SCHEMA_FIELD_TYPE_MISMATCH;
  } else {
    val.set_float(float_value);
  }
  return rc;
}

/**
 * @brief 把浮点 Value 序列化为字符串。
 * @param val 源值。
 * @param result 输出字符串。
 * @return 成功状态码。
 */
RC FloatType::to_string(const Value &val, string &result) const
{
  stringstream ss;
  ss << common::double_to_str(val.value_.float_value_);
  result = ss.str();
  return RC::SUCCESS;
}
