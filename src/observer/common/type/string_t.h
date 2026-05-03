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

#include <iostream>
#include <cstring>

using namespace std;

/**
 * @brief 轻量字符串视图/短串内联容器。
 * @details 长度不超过 INLINE_LENGTH 的字符串直接内联存储，超过阈值时只借用外部指针，
 * 因此它本身不拥有长字符串内存，只负责记录长度和访问入口。
 */
struct string_t
{
public:
  static constexpr int INLINE_LENGTH = 12;  ///< 可直接内联存储的最大字节数。

  string_t() = default;

  /// @brief 仅按长度初始化一个空字符串对象，供后续填充使用。
  explicit string_t(uint32_t len) { value.inlined.length = len; }

  /// @brief 使用一段字节序列初始化 string_t。
  string_t(const char *data, uint32_t len) { init(data, len); }

  /// @brief 重置内部状态；对于长串仅清空借用指针，不负责释放外部内存。
  ~string_t() { reset(); }

  /**
   * @brief 根据长度选择内联存储或外部指针模式。
   * @param data 输入字节地址。
   * @param len 输入长度。
   * @note 长字符串模式下只保存指针，调用方必须保证外部内存在 string_t 生命周期内有效。
   */
  void init(const char *data, uint32_t len)
  {
    value.inlined.length = len;
    if (is_inlined()) {
      // 短串直接复制到对象内部，避免额外堆分配和间接寻址。
      memset(value.inlined.inlined, 0, INLINE_LENGTH);
      if (size() == 0) {
        return;
      }
      memcpy(value.inlined.inlined, data, size());
    } else {
      // 长串模式只借用外部指针，因此 reset() 不会 delete 这段内存。
      value.pointer.ptr = (char *)data;
    }
  }

  /// @brief 清空当前对象内容，并恢复为长度 0 的空串状态。
  void reset()
  {
    if (is_inlined()) {
      memset(value.inlined.inlined, 0, INLINE_LENGTH);
    } else {
      value.pointer.ptr = nullptr;
    }
    value.inlined.length = 0;
  }

  string_t(const char *data) : string_t(data, strlen(data)) {}
  string_t(const string &value) : string_t(value.c_str(), value.size()) {}

  /// @brief 判断当前字符串是否采用内联存储。
  bool is_inlined() const { return size() <= INLINE_LENGTH; }

  /// @brief 返回只读数据指针。
  const char *data() const { return is_inlined() ? value.inlined.inlined : value.pointer.ptr; }

  /// @brief 返回可写数据指针；调用方需要自行保证写入不会破坏外部借用内存。
  char *get_data_writeable() const { return is_inlined() ? (char *)value.inlined.inlined : value.pointer.ptr; }

  /// @brief 返回当前字符串长度。
  int size() const { return value.inlined.length; }

  /// @brief 判断是否为空串。
  bool empty() const { return value.inlined.length == 0; }

  /// @brief 拷贝构造出标准 string。
  string get_string() const { return string(data(), size()); }

  /// @brief 比较两个 string_t 是否长度和字节内容都相等。
  bool operator==(const string_t &r) const
  {
    if (this->size() != r.size()) {
      return false;
    }
    return (memcmp(this->data(), r.data(), this->size()) == 0);
  }

  bool operator!=(const string_t &r) const { return !(*this == r); }

  /// @brief 按字节序和长度比较两个 string_t 的大小。
  bool operator>(const string_t &r) const
  {
    const uint32_t left_length  = this->size();
    const uint32_t right_length = r.size();
    const uint32_t min_length   = std::min<uint32_t>(left_length, right_length);

    auto memcmp_res = memcmp(this->data(), r.data(), min_length);
    return memcmp_res > 0 || (memcmp_res == 0 && left_length > right_length);
  }
  bool operator<(const string_t &r) const { return r > *this; }

  /// @brief 短串模式的内联存储布局。
  struct Inlined
  {
    uint32_t length;
    char     inlined[12];
  };

  /// @brief 长串与短串共用的联合体布局，首字段都放 length 以便统一读取 size()。
  union
  {
    struct
    {
      uint32_t length;
      char    *ptr;
    } pointer;
    Inlined inlined;
  } value;
};
