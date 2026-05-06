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

#include "common/lang/string.h"

namespace oceanbase {

// internal key 尾部固定追加一个 8 字节的序列号。
static const uint8_t SEQ_SIZE               = 8;
// lookup key 前面还会再带一个长度前缀，便于在 MemTable 里做 seek。
static const uint8_t LOOKUP_KEY_PREFIX_SIZE = 8;

/**
 * @brief 以二进制原样拷贝的方式把数值追加到字符串尾部。
 *
 * 这里不做人类可读编码，目的是让 MemTable / Block / Manifest 等内部结构
 * 在编码和解码时尽可能简单直接。
 */
template <typename T>
void put_numeric(string *dst, T v)
{
  dst->append(reinterpret_cast<char *>(&v), sizeof(T));
}

/**
 * @brief 从给定地址按二进制格式读出一个数值。
 */
template <typename T>
T get_numeric(const char *src)
{
  T value;
  memcpy(&value, src, sizeof(T));
  return value;
}

/**
 * @brief 从 internal key 中取出 user key 部分。
 *
 * internal key 布局：
 * `| user_key | seq(8B) |`
 */
inline string_view extract_user_key(const string_view &internal_key)
{
  return string_view(internal_key.data(), internal_key.size() - SEQ_SIZE);
}

/**
 * @brief 从 internal key 尾部取出 seq。
 */
inline uint64_t extract_sequence(const string_view &internal_key)
{
  return get_numeric<uint64_t>(internal_key.data() + internal_key.size() - SEQ_SIZE);
}

/**
 * @brief 计算 lookup key 中 user key 的长度。
 *
 * lookup key 布局：
 * `| internal_key_size(8B) | user_key | seq(8B) |`
 */
inline size_t user_key_size_from_lookup_key(const string_view &lookup_key)
{
  return lookup_key.size() - SEQ_SIZE - LOOKUP_KEY_PREFIX_SIZE;
}

/**
 * @brief 从 lookup key 中提取 user key。
 */
inline string_view extract_user_key_from_lookup_key(const string_view &lookup_key)
{
  return string_view(lookup_key.data() + LOOKUP_KEY_PREFIX_SIZE, user_key_size_from_lookup_key(lookup_key));
}

/**
 * @brief 从 lookup key 中提取 internal key。
 */
inline string_view extract_internal_key(const string_view &lookup_key)
{
  return string_view(lookup_key.data() + LOOKUP_KEY_PREFIX_SIZE, lookup_key.size() - LOOKUP_KEY_PREFIX_SIZE);
}

/**
 * @brief 读取一个“长度前缀 + 数据体”格式的切片。
 *
 * 常用于解析 MemTable 中编码后的 entry。
 */
inline string_view get_length_prefixed_string(const char *data)
{
  size_t      len = get_numeric<size_t>(data);
  const char *p   = data + sizeof(size_t);
  return string_view(p, len);
}

}  // namespace oceanbase
