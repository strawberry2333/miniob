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
#include "common/lang/vector.h"
#include "oblsm/include/ob_lsm_iterator.h"
#include "oblsm/util/ob_comparator.h"

namespace oceanbase {

// Block 是 SSTable 的最小读写单位。
// 目前 block 尾部维护了一段 offset 数组，用于在单个 block 内快速定位 entry。
// TODO: 后续可以考虑按 4KB 对齐，进一步贴近实际存储引擎的页组织方式。
//      ┌─────────────────┐
//      │    entry 1      │◄───┐
//      ├─────────────────┤    │
//      │    entry 2      │    │
//      ├─────────────────┤    │
//      │      ..         │    │
//      ├─────────────────┤    │
//      │    entry n      │◄─┐ │
//      ├─────────────────┤  │ │
// ┌───►│  offset size(n) │  │ │
// │    ├─────────────────┤  │ │
// │    │    offset 1     ├──┼─┘
// │    ├─────────────────┤  │
// │    │      ..         │  │
// │    ├─────────────────┤  │
// │    │    offset n     ├──┘
// │    ├─────────────────┤
// └────┤  offset start   │
//      └─────────────────┘
/**
 * @class ObBlock
 * @brief SSTable 内的一个数据块。
 *
 * 一个 block 内保存多个有序 entry，每个 entry 编码格式为：
 * `| key_size(uint32_t) | key | value_size(uint32_t) | value |`
 *
 * block 自身在尾部还维护一份 offset 表，用于定位每条 entry 的起始位置。
 * 因此 block 既兼顾了顺序遍历，也支持块内查找。
 *
 * 更完整地说，`ObBlockBuilder::finish()` 产出的序列化布局是：
 * `| entry_data... | entry_count | offset[0] ... offset[n-1] | data_start |`
 *
 * 其中：
 * - `entry_data` 区顺序存放所有 `| key_size | key | value_size | value |`；
 * - `offset[i]` 指向第 i 条 entry 在 `entry_data` 内的起始位置；
 * - `data_start` 记录 offset 表从哪里开始，便于解码时从尾部反向恢复结构。
 */
class ObBlock
{

public:
  ObBlock(const ObComparator *comparator) : comparator_(comparator) {}

  // 仅在解码或构造阶段维护 offset 表；普通读路径只依赖已有 offsets_。
  void add_offset(uint32_t offset) { offsets_.push_back(offset); }

  uint32_t get_offset(int index) const { return offsets_[index]; }

  /**
   * @brief 取出第 `offset` 条 entry 的原始字节切片。
   *
   * 注意这里的参数是“entry 下标”，不是 data_ 内的原始字节偏移。
   * 真正的字节边界通过 offsets_[offset] 与 offsets_[offset + 1] 推导。
   */
  string_view get_entry(uint32_t offset) const;

  int size() const { return offsets_.size(); }

  /**
   * @brief 从序列化字节流恢复 block。
   *
   * 这里会重新切出 data 区和 offset 区，使得后续迭代/查找可以直接在内存视图上工作。
   */
  RC decode(const string &data);

  ObLsmIterator *new_iterator() const;

private:
  string           data_;
  vector<uint32_t> offsets_;
  // 用于块内二分/线性查找时比较 key。
  const ObComparator *comparator_;
};

class BlockIterator : public ObLsmIterator
{
public:
  BlockIterator(const ObComparator *comparator, const ObBlock *data, uint32_t count)
      : comparator_(comparator), data_(data), count_(count)
  {}
  BlockIterator(const BlockIterator &)            = delete;
  BlockIterator &operator=(const BlockIterator &) = delete;

  ~BlockIterator() override = default;

  void seek(const string_view &lookup_key) override;
  /**
   * 块内迭代器只负责单个 block 范围内的有序扫描。
   * 跨块跳转由外层 `TableIterator` 完成，因此这里的 `next()` 到块尾就结束。
   */
  void seek_to_first() override
  {
    index_ = 0;
    parse_entry();
  }
  void seek_to_last() override
  {
    index_ = count_ - 1;
    parse_entry();
  }

  bool valid() const override { return index_ < count_; }
  void next() override
  {
    index_++;
    if (valid()) {
      parse_entry();
    }
  }
  string_view key() const override { return key_; };
  string_view value() const override { return value_; }

private:
  // 解析当前 index 指向的 entry，更新 key_/value_ 视图。
  // BlockIterator 不缓存解码后的对象，只缓存两个 string_view。
  void parse_entry();

private:
  const ObComparator  *comparator_;
  const ObBlock *const data_;
  string_view          curr_entry_;
  string_view          key_;
  string_view          value_;
  uint32_t             count_ = 0;
  uint32_t             index_ = 0;
};

class BlockMeta
{
public:
  BlockMeta() {}
  BlockMeta(const string &first_key, const string &last_key, uint32_t offset, uint32_t size)
      : first_key_(first_key), last_key_(last_key), offset_(offset), size_(size)
  {}
  string encode() const;
  RC     decode(const string &data);

  string first_key_;
  string last_key_;

  // 当前 block 在 SSTable 文件中的起始偏移和字节长度。
  // `first_key_/last_key_` 负责“范围裁剪”，`offset_/size_` 负责真正 I/O。
  uint32_t offset_;
  uint32_t size_;
};
}  // namespace oceanbase
