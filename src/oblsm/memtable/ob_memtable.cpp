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
  size_t       user_key_size          = key.size();
  size_t       val_size                = value.size();
  size_t       internal_key_size = user_key_size + SEQ_SIZE;
  const size_t encoded_len       = sizeof(size_t) + internal_key_size + sizeof(size_t) + val_size;
  char *       buf               = reinterpret_cast<char *>(arena_.alloc(encoded_len));
  char *       p                 = buf;

  // 1. 写 internal key 长度。
  memcpy(p, &internal_key_size, sizeof(size_t));
  p += sizeof(size_t);
  // 2. 写 user key。
  memcpy(p, key.data(), user_key_size);
  p += user_key_size;
  // 3. 紧跟在 user key 后写 seq，组合成完整 internal key。
  memcpy(p, &seq, sizeof(uint64_t));
  p += sizeof(uint64_t);
  // 4. 写 value 长度和值内容。
  memcpy(p, &val_size, sizeof(size_t));
  p += sizeof(size_t);
  memcpy(p, value.data(), val_size);

  // 跳表里仅保存 entry 首地址，比较时会从该地址反解出 internal key。
  table_.insert(buf);
}

int ObMemTable::KeyComparator::operator()(const char *a, const char *b) const
{
  // entry 起始位置先存了一个长度前缀，因此这里可以直接复用工具函数切出 internal key。
  string_view a_v = get_length_prefixed_string(a);
  string_view b_v = get_length_prefixed_string(b);
  return comparator.compare(a_v, b_v);
}

ObLsmIterator *ObMemTable::new_iterator() { return new ObMemTableIterator(get_shared_ptr(), &table_); }

string_view ObMemTableIterator::key() const
{
  // 先取出 internal key，对外返回的仍然是 internal key 视图。
  return get_length_prefixed_string(iter_.key());
}

string_view ObMemTableIterator::value() const
{
  // entry 中 value 紧跟在 internal key 之后，因此先取 internal key 再继续解析即可。
  string_view key_slice = get_length_prefixed_string(iter_.key());
  return get_length_prefixed_string(key_slice.data() + key_slice.size());
}

void ObMemTableIterator::seek(const string_view &k)
{
  tmp_.clear();
  // 跳表 iterator 的 seek 接口接受与表中 key 同构的对象。
  // 当前这里直接传入底层字节地址，依赖调用方构造好 lookup/internal key。
  iter_.seek(k.data());
}

}  // namespace oceanbase
