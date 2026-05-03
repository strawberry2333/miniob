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
// Created by Wangyunlai 2023/6/27
//

#pragma once

#include "common/lang/string.h"
#include "common/lang/memory.h"
#include "common/type/attr_type.h"
#include "common/type/data_type.h"
#include "common/type/string_t.h"

/**
 * @brief observer 运行时值对象定义。
 * @details Value 同时保存“逻辑类型 + 实际数据”，是表达式计算、记录读写和类型转换的核心载体。
 */

/**
 * @brief 属性的值
 * @ingroup DataType
 * @details 与DataType，就是数据类型，配套完成各种算术运算、比较、类型转换等操作。这里同时记录了数据的值与类型。
 * 当需要对值做运算时，建议使用类似 Value::add 的操作而不是 DataType::add。在进行运算前，应该设置好结果的类型，
 * 比如进行两个INT类型的除法运算时，结果类型应该设置为FLOAT。
 */
class Value final
{
public:
  friend class DataType;
  friend class IntegerType;
  friend class FloatType;
  friend class BooleanType;
  friend class CharType;
  friend class VectorType;

  /// @brief 构造一个未定义类型的空值。
  Value() = default;

  /// @brief 析构时按当前类型释放自己拥有的资源。
  ~Value() { reset(); }

  /// @brief 用原始内存初始化一个指定类型的值。
  Value(AttrType attr_type, char *data, int length = 4) : attr_type_(attr_type) { this->set_data(data, length); }

  /// @brief 构造整数值。
  explicit Value(int val);
  /// @brief 构造浮点值。
  explicit Value(float val);
  /// @brief 构造布尔值。
  explicit Value(bool val);
  /// @brief 构造字符串值；len 为 0 时按 C 字符串长度计算。
  explicit Value(const char *s, int len = 0);
  /// @brief 从 string_t 构造字符串值。
  explicit Value(const string_t &val);

  /// @brief 拷贝构造；对 CHARS 执行深拷贝。
  Value(const Value &other);
  /// @brief 移动构造；转移内部资源所有权。
  Value(Value &&other);

  /// @brief 拷贝赋值；会先释放当前资源，再按类型复制另一份值。
  Value &operator=(const Value &other);
  /// @brief 移动赋值；会先释放当前资源，再接管另一份值的资源。
  Value &operator=(Value &&other);

  /// @brief 清空当前值并释放自己拥有的资源。
  void reset();

  /// @brief 按结果类型分派加法实现。
  static RC add(const Value &left, const Value &right, Value &result)
  {
    return DataType::type_instance(result.attr_type())->add(left, right, result);
  }

  /// @brief 按结果类型分派减法实现。
  static RC subtract(const Value &left, const Value &right, Value &result)
  {
    return DataType::type_instance(result.attr_type())->subtract(left, right, result);
  }

  /// @brief 按结果类型分派乘法实现。
  static RC multiply(const Value &left, const Value &right, Value &result)
  {
    return DataType::type_instance(result.attr_type())->multiply(left, right, result);
  }

  /// @brief 按结果类型分派除法实现。
  static RC divide(const Value &left, const Value &right, Value &result)
  {
    return DataType::type_instance(result.attr_type())->divide(left, right, result);
  }

  /// @brief 按结果类型分派一元负号实现。
  static RC negative(const Value &value, Value &result)
  {
    return DataType::type_instance(result.attr_type())->negative(value, result);
  }

  /// @brief 根据源值类型分派类型转换实现。
  static RC cast_to(const Value &value, AttrType to_type, Value &result)
  {
    return DataType::type_instance(value.attr_type())->cast_to(value, to_type, result);
  }

  /// @brief 直接覆盖当前值的逻辑类型。
  void set_type(AttrType type) { this->attr_type_ = type; }
  /// @brief 从一段原始内存中解析出当前类型对应的值。
  void set_data(char *data, int length);
  void set_data(const char *data, int length) { this->set_data(const_cast<char *>(data), length); }
  /// @brief 按源值的逻辑类型复制实际内容。
  void set_value(const Value &value);
  /// @brief 把当前值设置成布尔值。
  void set_boolean(bool val);

  /// @brief 把当前值格式化成可读字符串。
  string to_string() const;

  /// @brief 按当前类型语义比较自己与另一个值。
  int compare(const Value &other) const;

  /// @brief 返回底层数据指针；定长类型直接指向 union，字符串返回 char*。
  char *data() const;

  int      length() const { return length_; }
  AttrType attr_type() const { return attr_type_; }

public:
  /**
   * 获取对应的值
   * 如果当前的类型与期望获取的类型不符，就会执行转换操作
   */
  int      get_int() const;
  float    get_float() const;
  string   get_string() const;
  string_t get_string_t() const;
  bool     get_boolean() const;

public:
  /// @brief 把当前值重置为整数。
  void set_int(int val);
  /// @brief 把当前值重置为浮点数。
  void set_float(float val);
  /// @brief 把当前值重置为字符串。
  void set_string(const char *s, int len = 0);
  /// @brief 分配一个指定长度的空字符串缓冲区，并用 `\0` 填充。
  void set_empty_string(int len);
  /// @brief 从另一个 CHARS 值深拷贝字符串内容。
  void set_string_from_other(const Value &other);

private:
  AttrType attr_type_ = AttrType::UNDEFINED;  ///< 当前值的逻辑类型
  int      length_    = 0;                    ///< 数据长度；对定长类型通常等于 sizeof(type)

  union Val
  {
    int32_t int_value_;
    float   float_value_;
    bool    bool_value_;
    char   *pointer_value_;
  } value_ = {.int_value_ = 0};

  /// 是否申请并占有内存, 目前对于 CHARS 类型 own_data_ 为true, 其余类型 own_data_ 为false
  bool own_data_ = false;  ///< 当前 Value 是否拥有并负责释放 pointer_value_ 指向的内存
};
