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

// internal key 尾部固定追加一个 8 字节序列号，形成：
// `| user_key | seq(8B) |`
static const uint8_t SEQ_SIZE               = 8;
// lookup key 额外在最前面带一个 8 字节长度前缀，形成：
// `| internal_key_size(8B) | user_key | seq(8B) |`
// 这样在 MemTable 中可以直接按“长度前缀字符串”查找。
static const uint8_t LOOKUP_KEY_PREFIX_SIZE = 8;

/**
 * @brief 以二进制原样拷贝的方式把数值追加到字符串尾部。
 *
 * 这里不做人类可读编码，目的是让 MemTable / Block / Manifest 等内部结构
 * 在编码和解码时尽可能简单直接。
 *
 * @note
 * 1. 这里写入的是宿主机内存中的原始字节表示；
 * 2. 因而它适合进程内或同构环境下的内部持久化格式；
 * 3. 如果未来要做跨平台兼容，需要额外约定字节序和整数宽度。
 */
template <typename T>
void put_numeric(string *dst, T v)
{
  dst->append(reinterpret_cast<char *>(&v), sizeof(T));
}

/**
 * @brief 从给定地址按二进制格式读出一个数值。
 *
 * 调用方必须保证 `src` 指向至少 `sizeof(T)` 字节的连续内存。
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
 *
 * 返回值只是视图，不会拷贝底层字节。
 */
inline string_view extract_user_key(const string_view &internal_key)
{
  return string_view(internal_key.data(), internal_key.size() - SEQ_SIZE);
}

/**
 * @brief 从 internal key 尾部取出 seq。
 *
 * 约定 seq 存放在最后 8 个字节中，便于比较器在 user key 相同的前提下
 * 继续按版本号排序。
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
 *
 * 这里直接用总长度减去前缀和尾部 seq 长度，因此要求传入的 lookup key
 * 已经满足 oblsm 内部约定格式。
 */
inline size_t user_key_size_from_lookup_key(const string_view &lookup_key)
{
  return lookup_key.size() - SEQ_SIZE - LOOKUP_KEY_PREFIX_SIZE;
}

/**
 * @brief 从 lookup key 中提取 user key。
 *
 * 这个视图会跳过最前面的 internal key 长度前缀，也会忽略尾部的 seq。
 */
inline string_view extract_user_key_from_lookup_key(const string_view &lookup_key)
{
  return string_view(lookup_key.data() + LOOKUP_KEY_PREFIX_SIZE, user_key_size_from_lookup_key(lookup_key));
}

/**
 * @brief 从 lookup key 中提取 internal key。
 *
 * 结果视图对应 `user_key + seq` 整体，常用于直接交给 internal comparator。
 */
inline string_view extract_internal_key(const string_view &lookup_key)
{
  return string_view(lookup_key.data() + LOOKUP_KEY_PREFIX_SIZE, lookup_key.size() - LOOKUP_KEY_PREFIX_SIZE);
}

/**
 * @brief 读取一个“长度前缀 + 数据体”格式的切片。
 *
 * 常用于解析 MemTable 中编码后的 entry。
 * 这里的长度前缀类型是 `size_t`，因此编码端和解码端需要共享同样的字长约定。
 */
inline string_view get_length_prefixed_string(const char *data)
{
  size_t      len = get_numeric<size_t>(data);
  const char *p   = data + sizeof(size_t);
  return string_view(p, len);
}

}  // namespace oceanbase
