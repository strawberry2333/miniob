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
 * @brief 向量类型处理器定义。
 * @details 当前只是类型占位符，用于为后续向量存储/检索能力预留枚举和值分派入口。
 */

/**
 * @brief 向量类型
 * @ingroup DataType
 */
class VectorType : public DataType
{
public:
  VectorType() : DataType(AttrType::VECTORS) {}
  virtual ~VectorType() {}

  /// @brief 当前尚未定义向量比较语义，返回未实现标记。
  int compare(const Value &left, const Value &right) const override { return INT32_MAX; }

  /// @brief 当前未实现向量加法。
  RC add(const Value &left, const Value &right, Value &result) const override { return RC::UNIMPLEMENTED; }
  /// @brief 当前未实现向量减法。
  RC subtract(const Value &left, const Value &right, Value &result) const override { return RC::UNIMPLEMENTED; }
  /// @brief 当前未实现向量乘法。
  RC multiply(const Value &left, const Value &right, Value &result) const override { return RC::UNIMPLEMENTED; }

  /// @brief 当前未实现向量字符串化。
  RC to_string(const Value &val, string &result) const override { return RC::UNIMPLEMENTED; }
};
