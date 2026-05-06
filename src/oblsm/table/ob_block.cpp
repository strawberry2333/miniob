/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "oblsm/table/ob_block.h"
#include "oblsm/util/ob_coding.h"
#include "common/lang/memory.h"

namespace oceanbase {

RC ObBlock::decode(const string &data)
{
  // TODO: 当前 block 解码尚未实现。
  // 未来应按 `ObBlockBuilder::finish()` 的输出格式恢复：
  // 1. 末尾 data_start；
  // 2. offset 数量；
  // 3. offset 数组；
  // 4. entry 数据区。
  // 解码完成后应把：
  // - `data_` 指向纯 entry 数据区；
  // - `offsets_` 恢复为每条 entry 的逻辑下标 -> 字节偏移映射；
  // 这样上层块内迭代器就无需感知底层序列化细节。
  return RC::UNIMPLEMENTED;
}

string_view ObBlock::get_entry(uint32_t offset) const
{
  // 根据 offset 表取出第 offset 条 entry 的字节区间。
  // 最后一条 entry 的结束位置不在 offset 表里单独保存，因此以 data_ 末尾作为右边界。
  uint32_t    curr_begin = offsets_[offset];
  uint32_t    curr_end   = offset == offsets_.size() - 1 ? data_.size() : offsets_[offset + 1];
  string_view curr       = string_view(data_.data() + curr_begin, curr_end - curr_begin);
  return curr;
}

ObLsmIterator *ObBlock::new_iterator() const { return new BlockIterator(comparator_, this, size()); }

void BlockIterator::parse_entry()
{
  // entry 布局：| key_size | key | value_size | value |
  // 解析结果直接指向 block 内存；TableIterator 切换 block 后这些视图会一起更新。
  curr_entry_         = data_->get_entry(index_);
  uint32_t key_size   = get_numeric<uint32_t>(curr_entry_.data());
  key_                = string_view(curr_entry_.data() + sizeof(uint32_t), key_size);
  uint32_t value_size = get_numeric<uint32_t>(curr_entry_.data() + sizeof(uint32_t) + key_size);
  value_              = string_view(curr_entry_.data() + 2 * sizeof(uint32_t) + key_size, value_size);
}

string BlockMeta::encode() const
{
  // block meta 需要记住首尾 key 和块在文件中的位置信息，
  // 这样 TableIterator 才能先靠 meta 做粗粒度定位，再真正读 block。
  // 这部分元数据通常位于 SSTable 文件尾部，读取表时会先整体加载到内存。
  string ret;
  put_numeric<uint32_t>(&ret, first_key_.size());
  ret.append(first_key_);
  put_numeric<uint32_t>(&ret, last_key_.size());
  ret.append(last_key_);
  put_numeric<uint32_t>(&ret, offset_);
  put_numeric<uint32_t>(&ret, size_);
  return ret;
}

RC BlockMeta::decode(const string &data)
{
  RC rc = RC::SUCCESS;
  const char *data_ptr       = data.c_str();
  uint32_t    first_key_size = get_numeric<uint32_t>(data_ptr);
  data_ptr += sizeof(uint32_t);
  first_key_.assign(data_ptr, first_key_size);
  data_ptr += first_key_size;
  uint32_t last_key_size = get_numeric<uint32_t>(data_ptr);
  data_ptr += sizeof(uint32_t);
  last_key_.assign(data_ptr, last_key_size);
  data_ptr += last_key_size;
  offset_ = get_numeric<uint32_t>(data_ptr);
  data_ptr += sizeof(uint32_t);
  size_ = get_numeric<uint32_t>(data_ptr);
  return rc;
}

void BlockIterator::seek(const string_view &lookup_key)
{
   // 当前实现是线性扫描；块不大时实现最简单。
   // 后续可以利用块内有序性改成二分查找。
   // 注意比较时会把 block 内 entry 的 internal key 降成 user key，
   // 再与 lookup key 中的 user key 比较，这样块内定位只按用户键范围进行。
   index_ = 0;
   while (valid()) {
    parse_entry();
    if (comparator_->compare(extract_user_key(key_), extract_user_key_from_lookup_key(lookup_key)) >= 0) {
      break;
    }
    index_++;
   }
}
}  // namespace oceanbase
