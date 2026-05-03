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
// Created by WangYunlai on 2023/06/28.
//

#include "common/value.h"

#include "common/lang/comparator.h"
#include "common/lang/exception.h"
#include "common/lang/sstream.h"
#include "common/lang/string.h"
#include "common/log/log.h"

/**
 * @brief Value 的实现文件。
 * @details 这里既处理不同逻辑类型的内存布局，也负责值语义复制、隐式转换和字符串解析。
 */

/// @brief 直接把整数写入内部 union，并切换类型。
Value::Value(int val) { set_int(val); }

/// @brief 直接把浮点数写入内部 union，并切换类型。
Value::Value(float val) { set_float(val); }

/// @brief 直接把布尔值写入内部 union，并切换类型。
Value::Value(bool val) { set_boolean(val); }

/// @brief 使用一段字符数据构造 CHARS 值。
Value::Value(const char *s, int len /*= 0*/) { set_string(s, len); }

/// @brief 从 string_t 拷贝出一份独立的 CHARS 值。
Value::Value(const string_t& s) { set_string(s.data(), s.size()); }


/**
 * @brief 拷贝构造一个 Value。
 * @param other 源值。
 * @note 对 CHARS 执行深拷贝，避免两个 Value 共用同一段可释放内存。
 */
Value::Value(const Value &other)
{
  this->attr_type_ = other.attr_type_;
  this->length_    = other.length_;
  this->own_data_  = other.own_data_;
  switch (this->attr_type_) {
    case AttrType::CHARS: {
      // 字符串类型需要重新分配一份内存，保持值语义。
      set_string_from_other(other);
    } break;

    default: {
      this->value_ = other.value_;
    } break;
  }
}

/**
 * @brief 移动构造一个 Value。
 * @param other 源值。
 * @note 这里只转移所有权，不复制字符串内容。
 */
Value::Value(Value &&other)
{
  this->attr_type_ = other.attr_type_;
  this->length_    = other.length_;
  this->own_data_  = other.own_data_;
  this->value_     = other.value_;
  other.own_data_  = false;
  other.length_    = 0;
}

/**
 * @brief 拷贝赋值。
 * @param other 源值。
 * @return 当前对象自身。
 */
Value &Value::operator=(const Value &other)
{
  if (this == &other) {
    return *this;
  }
  reset();
  this->attr_type_ = other.attr_type_;
  this->length_    = other.length_;
  this->own_data_  = other.own_data_;
  switch (this->attr_type_) {
    case AttrType::CHARS: {
      // 保持字符串类型的深拷贝语义。
      set_string_from_other(other);
    } break;

    default: {
      this->value_ = other.value_;
    } break;
  }
  return *this;
}

/**
 * @brief 移动赋值。
 * @param other 源值。
 * @return 当前对象自身。
 */
Value &Value::operator=(Value &&other)
{
  if (this == &other) {
    return *this;
  }
  reset();
  this->attr_type_ = other.attr_type_;
  this->length_    = other.length_;
  this->own_data_  = other.own_data_;
  this->value_     = other.value_;
  other.own_data_  = false;
  other.length_    = 0;
  return *this;
}

/**
 * @brief 释放当前值占用的资源并恢复为空值状态。
 * @note 目前只有 CHARS 类型可能真正持有堆内存。
 */
void Value::reset()
{
  switch (attr_type_) {
    case AttrType::CHARS:
      if (own_data_ && value_.pointer_value_ != nullptr) {
        delete[] value_.pointer_value_;
        value_.pointer_value_ = nullptr;
      }
      break;
    default: break;
  }

  attr_type_ = AttrType::UNDEFINED;
  length_    = 0;
  own_data_  = false;
}

/**
 * @brief 按当前逻辑类型从原始内存加载值。
 * @param data 原始字节地址。
 * @param length 输入长度。
 * @note 调用前必须先正确设置 attr_type_，否则无法知道如何解释这段内存。
 */
void Value::set_data(char *data, int length)
{
  switch (attr_type_) {
    case AttrType::CHARS: {
      set_string(data, length);
    } break;
    case AttrType::INTS: {
      // 定长类型直接按目标类型解引用即可。
      value_.int_value_ = *(int *)data;
      length_           = length;
    } break;
    case AttrType::FLOATS: {
      value_.float_value_ = *(float *)data;
      length_             = length;
    } break;
    case AttrType::BOOLEANS: {
      value_.bool_value_ = *(int *)data != 0;
      length_            = length;
    } break;
    default: {
      LOG_WARN("unknown data type: %d", attr_type_);
    } break;
  }
}

/// @brief 把当前值重置为整数。
void Value::set_int(int val)
{
  reset();
  attr_type_        = AttrType::INTS;
  value_.int_value_ = val;
  length_           = sizeof(val);
}

/// @brief 把当前值重置为浮点数。
void Value::set_float(float val)
{
  reset();
  attr_type_          = AttrType::FLOATS;
  value_.float_value_ = val;
  length_             = sizeof(val);
}
/// @brief 把当前值重置为布尔值。
void Value::set_boolean(bool val)
{
  reset();
  attr_type_         = AttrType::BOOLEANS;
  value_.bool_value_ = val;
  length_            = sizeof(val);
}

/**
 * @brief 把当前值重置为字符串。
 * @param s 输入字符串地址。
 * @param len 最大可读取长度；为 0 时按 C 字符串长度读取。
 * @note 这里总是为 CHARS 值分配一份独立内存，因此 Value 对字符串拥有释放责任。
 */
void Value::set_string(const char *s, int len /*= 0*/)
{
  reset();
  attr_type_ = AttrType::CHARS;
  if (s == nullptr) {
    value_.pointer_value_ = nullptr;
    length_               = 0;
  } else {
    own_data_ = true;
    if (len > 0) {
      // 对定长字符列仍然只保留有效内容长度，忽略尾部无意义的填充字节。
      len = strnlen(s, len);
    } else {
      len = strlen(s);
    }
    value_.pointer_value_ = new char[len + 1];
    length_               = len;
    memcpy(value_.pointer_value_, s, len);
    value_.pointer_value_[len] = '\0';
  }
}

/**
 * @brief 分配一个指定长度的空字符串缓冲区。
 * @param len 目标字符串长度。
 */
void Value::set_empty_string(int len)
{
  reset();
  attr_type_ = AttrType::CHARS;

  own_data_ = true;
  value_.pointer_value_ = new char[len + 1];
  length_               = len;
  memset(value_.pointer_value_, 0, len);
  value_.pointer_value_[len] = '\0';
}

/**
 * @brief 按源值类型复制内容到当前 Value。
 * @param value 源值。
 * @note 该接口会根据源值的逻辑类型重新走一遍 set_int/set_float/set_string 等路径。
 */
void Value::set_value(const Value &value)
{
  switch (value.attr_type_) {
    case AttrType::INTS: {
      set_int(value.get_int());
    } break;
    case AttrType::FLOATS: {
      set_float(value.get_float());
    } break;
    case AttrType::CHARS: {
      set_string(value.get_string().c_str());
    } break;
    case AttrType::BOOLEANS: {
      set_boolean(value.get_boolean());
    } break;
    default: {
      ASSERT(false, "got an invalid value type");
    } break;
  }
}

/**
 * @brief 从另一个 CHARS Value 深拷贝字符串内容。
 * @param other 源值，必须是 CHARS。
 */
void Value::set_string_from_other(const Value &other)
{
  ASSERT(attr_type_ == AttrType::CHARS, "attr type is not CHARS");
  if (own_data_ && other.value_.pointer_value_ != nullptr && length_ != 0) {
    // 重新申请缓冲区，确保两个 Value 各自拥有自己的字符串副本。
    this->value_.pointer_value_ = new char[this->length_ + 1];
    memcpy(this->value_.pointer_value_, other.value_.pointer_value_, this->length_);
    this->value_.pointer_value_[this->length_] = '\0';
  }
}

/**
 * @brief 返回底层数据指针。
 * @return CHARS 返回 char*，其余定长类型返回 union 首地址。
 */
char *Value::data() const
{
  switch (attr_type_) {
    case AttrType::CHARS: {
      return value_.pointer_value_;
    } break;
    default: {
      return (char *)&value_;
    } break;
  }
}

/**
 * @brief 把当前值格式化成字符串。
 * @return 可读字符串；若类型实例不支持格式化则返回空串。
 */
string Value::to_string() const
{
  string res;
  RC     rc = DataType::type_instance(this->attr_type_)->to_string(*this, res);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to convert value to string. type=%s", attr_type_to_string(this->attr_type_));
    return "";
  }
  return res;
}

/// @brief 按当前值的逻辑类型语义比较自己与另一个值。
int Value::compare(const Value &other) const { return DataType::type_instance(this->attr_type_)->compare(*this, other); }

/**
 * @brief 以整数语义读取当前值。
 * @return 读取到的整数；转换失败时返回 0。
 */
int Value::get_int() const
{
  switch (attr_type_) {
    case AttrType::CHARS: {
      try {
        // 字符串转整数失败时按 0 处理，并在 trace 日志中保留原因。
        return (int)(stol(value_.pointer_value_));
      } catch (exception const &ex) {
        LOG_TRACE("failed to convert string to number. s=%s, ex=%s", value_.pointer_value_, ex.what());
        return 0;
      }
    }
    case AttrType::INTS: {
      return value_.int_value_;
    }
    case AttrType::FLOATS: {
      return (int)(value_.float_value_);
    }
    case AttrType::BOOLEANS: {
      return (int)(value_.bool_value_);
    }
    default: {
      LOG_WARN("unknown data type. type=%d", attr_type_);
      return 0;
    }
  }
  return 0;
}

/**
 * @brief 以浮点语义读取当前值。
 * @return 读取到的浮点数；转换失败时返回 0。
 */
float Value::get_float() const
{
  switch (attr_type_) {
    case AttrType::CHARS: {
      try {
        return stof(value_.pointer_value_);
      } catch (exception const &ex) {
        LOG_TRACE("failed to convert string to float. s=%s, ex=%s", value_.pointer_value_, ex.what());
        return 0.0;
      }
    } break;
    case AttrType::INTS: {
      return float(value_.int_value_);
    } break;
    case AttrType::FLOATS: {
      return value_.float_value_;
    } break;
    case AttrType::BOOLEANS: {
      return float(value_.bool_value_);
    } break;
    default: {
      LOG_WARN("unknown data type. type=%d", attr_type_);
      return 0;
    }
  }
  return 0;
}

/// @brief 以字符串形式读取当前值。
string Value::get_string() const { return this->to_string(); }

/**
 * @brief 以 string_t 视图读取当前字符串值。
 * @return 一个借用当前字符串缓冲区的 string_t。
 * @note 只有在 attr_type_ 为 CHARS 时才合法。
 */
string_t Value::get_string_t() const
{
  ASSERT(attr_type_ == AttrType::CHARS, "attr type is not CHARS");
  return string_t(value_.pointer_value_, length_);
}

/**
 * @brief 以布尔语义读取当前值。
 * @return 转换后的布尔值。
 */
bool Value::get_boolean() const
{
  switch (attr_type_) {
    case AttrType::CHARS: {
      try {
        float val = stof(value_.pointer_value_);
        if (val >= EPSILON || val <= -EPSILON) {
          return true;
        }

        // 浮点解释为 0 后，再尝试按整数理解，兼容 "0"、"1" 这类常见输入。
        int int_val = stol(value_.pointer_value_);
        if (int_val != 0) {
          return true;
        }

        // 到这里说明字符串不是非零数字，只要指针存在就视作 true。
        return value_.pointer_value_ != nullptr;
      } catch (exception const &ex) {
        LOG_TRACE("failed to convert string to float or integer. s=%s, ex=%s", value_.pointer_value_, ex.what());
        return value_.pointer_value_ != nullptr;
      }
    } break;
    case AttrType::INTS: {
      return value_.int_value_ != 0;
    } break;
    case AttrType::FLOATS: {
      float val = value_.float_value_;
      return val >= EPSILON || val <= -EPSILON;
    } break;
    case AttrType::BOOLEANS: {
      return value_.bool_value_;
    } break;
    default: {
      LOG_WARN("unknown data type. type=%d", attr_type_);
      return false;
    }
  }
  return false;
}
