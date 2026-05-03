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
#include "common/log/log.h"
#include "common/type/char_type.h"
#include "common/value.h"

/**
 * @brief CharType 的实现。
 * @details 这里按 MiniOB 的字符串布局直接比较和转换 CHARS 类型的 Value。
 */

/**
 * @brief 比较两个 CHARS 值。
 * @param left 左操作数，必须是 CHARS。
 * @param right 右操作数，必须是 CHARS。
 * @return 标准三路比较结果。
 */
int CharType::compare(const Value &left, const Value &right) const
{
  ASSERT(left.attr_type() == AttrType::CHARS && right.attr_type() == AttrType::CHARS, "invalid type");
  return common::compare_string(
      (void *)left.value_.pointer_value_, left.length_, (void *)right.value_.pointer_value_, right.length_);
}

/**
 * @brief 从普通字符串初始化一个 CHARS Value。
 * @param val 待写入的 Value。
 * @param data 输入文本。
 * @return 始终返回成功。
 */
RC CharType::set_value_from_str(Value &val, const string &data) const
{
  val.set_string(data.c_str());
  return RC::SUCCESS;
}

/**
 * @brief 把 CHARS 转换到其它类型。
 * @param val 源值。
 * @param type 目标类型。
 * @param result 转换结果。
 * @return 当前仅保留未实现分支。
 */
RC CharType::cast_to(const Value &val, AttrType type, Value &result) const
{
  switch (type) {
    default: return RC::UNIMPLEMENTED;
  }
  return RC::SUCCESS;
}

/**
 * @brief 计算 CHARS 到目标类型的隐式转换代价。
 * @param type 目标类型。
 * @return 相同类型为 0，其余返回不可转换。
 */
int CharType::cast_cost(AttrType type)
{
  if (type == AttrType::CHARS) {
    return 0;
  }
  return INT32_MAX;
}

/**
 * @brief 把 CHARS Value 转回 string。
 * @param val 源值。
 * @param result 输出字符串。
 * @return 成功状态码。
 */
RC CharType::to_string(const Value &val, string &result) const
{
  stringstream ss;
  ss << val.value_.pointer_value_;
  result = ss.str();
  return RC::SUCCESS;
}
