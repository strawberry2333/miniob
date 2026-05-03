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

#include "common/lang/array.h"
#include "common/lang/memory.h"
#include "common/lang/string.h"
#include "common/sys/rc.h"
#include "common/type/attr_type.h"

class Value;
class Column;

/**
 * @brief observer 类型系统的抽象基类定义。
 * @details 每个具体类型都通过继承 DataType 实现比较、算术运算、类型转换和字符串化逻辑。
 */

/**
 * @brief 定义了数据类型相关的操作，比如比较运算、算术运算等
 * @defgroup DataType
 * @details 数据类型定义的算术运算中，比如 add、subtract 等，将按照当前数据类型设置最终结果值的类型。
 * 参与运算的参数类型不一定相同，不同的类型进行运算是否能够支持需要参考各个类型的实现。
 */

class DataType
{
public:
  /// @brief 绑定一个逻辑类型枚举，供运行时分派使用。
  explicit DataType(AttrType attr_type) : attr_type_(attr_type) {}

  virtual ~DataType() = default;

  /// @brief 根据 AttrType 取得对应的全局类型实例。
  inline static DataType *type_instance(AttrType attr_type)
  {
    return type_instances_.at(static_cast<int>(attr_type)).get();
  }

  /// @brief 返回当前类型实例所代表的 AttrType。
  inline AttrType get_attr_type() const { return attr_type_; }

  /**
   * @return
   *  -1 表示 left < right
   *  0 表示 left = right
   *  1 表示 left > right
   *  INT32_MAX 表示未实现的比较
   */
  /// @brief 比较两个 Value，返回 -1/0/1；默认表示未实现。
  virtual int compare(const Value &left, const Value &right) const { return INT32_MAX; }

  /// @brief 比较列式存储中的两个值；默认表示未实现。
  virtual int compare(const Column &left, const Column &right, int left_idx, int right_idx) const { return INT32_MAX; }

  /**
   * @brief 计算 left + right，并将结果保存到 result 中
   */
  virtual RC add(const Value &left, const Value &right, Value &result) const { return RC::UNSUPPORTED; }

  /**
   * @brief 计算 left - right，并将结果保存到 result 中
   */
  virtual RC subtract(const Value &left, const Value &right, Value &result) const { return RC::UNSUPPORTED; }

  /**
   * @brief 计算 left * right，并将结果保存到 result 中
   */
  virtual RC multiply(const Value &left, const Value &right, Value &result) const { return RC::UNSUPPORTED; }

  /**
   * @brief 计算 left / right，并将结果保存到 result 中
   */
  virtual RC divide(const Value &left, const Value &right, Value &result) const { return RC::UNSUPPORTED; }

  /**
   * @brief 计算 -val，并将结果保存到 result 中
   */
  virtual RC negative(const Value &val, Value &result) const { return RC::UNSUPPORTED; }

  /**
   * @brief 将 val 转换为 type 类型，并将结果保存到 result 中
   */
  virtual RC cast_to(const Value &val, AttrType type, Value &result) const { return RC::UNSUPPORTED; }

  /**
   * @brief 将 val 转换为 string，并将结果保存到 result 中
   */
  virtual RC to_string(const Value &val, string &result) const { return RC::UNSUPPORTED; }

  /**
   * @brief 计算从 type 到 attr_type 的隐式转换的 cost，如果无法转换，返回 INT32_MAX
   */
  virtual int cast_cost(AttrType type)
  {
    if (type == attr_type_) {
      return 0;
    }
    return INT32_MAX;
  }

  /// @brief 从字符串反序列化出一个 Value；默认不支持。
  virtual RC set_value_from_str(Value &val, const string &data) const { return RC::UNSUPPORTED; }

protected:
  AttrType attr_type_;

  static array<unique_ptr<DataType>, static_cast<int>(AttrType::MAXTYPE)> type_instances_;  ///< 全局共享的类型实例表
};
