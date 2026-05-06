/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "oblsm/memtable/ob_memtable.h"
#include "common/lang/string.h"
#include "common/lang/memory.h"
#include "oblsm/util/ob_coding.h"
#include "oblsm/ob_lsm_define.h"

namespace oceanbase {

void ObMemTable::put(uint64_t seq, const string_view &key, const string_view &value)
{
  // MemTable entry 的布局如下：
  // | internal_key_size(size_t) | user_key | seq(uint64_t) | value_size(size_t) | value |
  //
  // 其中 internal_key_size = user_key.size + sizeof(seq)。
  // 跳表排序时会把 `user_key + seq` 视为 internal key，并按“user_key 正序 + seq 倒序”比较。
  // 这意味着：
  // 1. 同一个 user key 的多个版本会相邻排布；
  // 2. 较新的 seq 会更早被迭代器看见；
  // 3. flush 时无需额外整理版本顺序，直接按跳表顺序输出即可。
  size_t       user_key_size          = key.size();
  size_t       val_size                = value.size();
  size_t       internal_key_size = user_key_size + SEQ_SIZE;
  const size_t encoded_len       = sizeof(size_t) + internal_key_size + sizeof(size_t) + val_size;
  char *       buf               = reinterpret_cast<char *>(arena_.alloc(encoded_len));
  char *       p                 = buf;

  // 1. 写 internal key 长度。
  //    读取侧会先依靠这个长度切出 `user_key + seq`，再继续解析 value。
  memcpy(p, &internal_key_size, sizeof(size_t));
  p += sizeof(size_t);
  // 2. 写 user key。
  //    user key 直接按原始字节拷贝，不再额外编码。
  memcpy(p, key.data(), user_key_size);
  p += user_key_size;
  // 3. 紧跟在 user key 后写 seq，组合成完整 internal key。
  //    这样同一 user key 的多个版本会天然共享前缀并排存放。
  memcpy(p, &seq, sizeof(uint64_t));
  p += sizeof(uint64_t);
  // 4. 写 value 长度和值内容。
  //    value 也以内联方式紧跟在 entry 后半段，避免二次寻址。
  memcpy(p, &val_size, sizeof(size_t));
  p += sizeof(size_t);
  memcpy(p, value.data(), val_size);

  // 跳表里仅保存 entry 首地址，比较时会从该地址反解出 internal key。
  // 因而一个节点即可同时承载排序键和返回值。
  table_.insert(buf);
}

int ObMemTable::KeyComparator::operator()(const char *a, const char *b) const
{
  // entry 起始位置先存了一个长度前缀，因此这里可以直接复用工具函数切出 internal key。
  // 比较器完全不关心 value，只关心 entry 前半段的 internal key 排序结果。
  // 也正因为如此，即便不同 entry 的 value 完全不同，只要 internal key 的顺序确定，
  // MemTable 的有序性就已经确定。
  string_view a_v = get_length_prefixed_string(a);
  string_view b_v = get_length_prefixed_string(b);
  return comparator.compare(a_v, b_v);
}

ObLsmIterator *ObMemTable::new_iterator() { return new ObMemTableIterator(get_shared_ptr(), &table_); }

string_view ObMemTableIterator::key() const
{
  // 先取出 internal key，对外返回的仍然是 internal key 视图。
  // 更上层如果只关心 user key，需要再调用 `extract_user_key()`。
  // 这里没有任何数据复制，返回值直接指向 arena 中保存的 entry。
  return get_length_prefixed_string(iter_.key());
}

string_view ObMemTableIterator::value() const
{
  // entry 中 value 紧跟在 internal key 之后，因此先取 internal key 再继续解析即可。
  // 这里复用了同一段原始字节，不会发生额外拷贝。
  string_view key_slice = get_length_prefixed_string(iter_.key());
  return get_length_prefixed_string(key_slice.data() + key_slice.size());
}

void ObMemTableIterator::seek(const string_view &k)
{
  tmp_.clear();
  // 跳表 iterator 的 seek 接口接受与表中 key 同构的对象。
  // 当前这里直接传入底层字节地址，依赖调用方构造好 lookup/internal key。
  // 常见调用方会传入 lookup key，使 seek 最终落到第一个 `>= 目标版本键` 的 entry。
  // 也就是说，seek 的比较单位仍是 internal key，而不是裸 user key。
  iter_.seek(k.data());
}

}  // namespace oceanbase
