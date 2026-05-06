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

#include "oblsm/util/ob_file_reader.h"
#include "common/lang/memory.h"
#include "common/sys/rc.h"
#include "oblsm/table/ob_block.h"
#include "oblsm/util/ob_comparator.h"
#include "oblsm/util/ob_lru_cache.h"

namespace oceanbase {
// TODO: 后续可以增加单独的 dumptool，把 SST 文件内容可视化输出。
//    ┌─────────────────┐
//    │    block 1      │◄──┐
//    ├─────────────────┤   │
//    │    block 2      │   │
//    ├─────────────────┤   │
//    │      ..         │   │
//    ├─────────────────┤   │
//    │    block n      │◄┐ │
//    ├─────────────────┤ │ │
// ┌─►│  meta size(n)   │ │ │
// │  ├─────────────────┤ │ │
// │  │block meta 1 size│ │ │
// │  ├─────────────────┤ │ │
// │  │  block meta 1   ┼─┼─┘
// │  ├─────────────────┤ │
// │  │      ..         │ │
// │  ├─────────────────┤ │
// │  │block meta n size│ │
// │  ├─────────────────┤ │
// │  │  block meta n   ┼─┘
// │  ├─────────────────┤
// └──┼                 │
//    └─────────────────┘

/**
 * @class ObSSTable
 * @brief 磁盘上的只读有序表。
 *
 * SSTable 是 MemTable flush 或 compaction 的结果，一旦生成后就不再原地修改。
 * 当前类主要负责三件事：
 * 1. 打开文件并解析 block meta；
 * 2. 按 block 读取数据，并尽量命中 block cache；
 * 3. 提供遍历接口，参与查询和 compaction。
 *
 * 读路径大致分成两级：
 * 1. 先用 `block_metas_` 做块级定位，判断目标 key 可能落在哪个 block；
 * 2. 再把对应 block 读入内存，交给 `BlockIterator` 做块内定位。
 *
 * 这样做的原因是 SSTable 可能远大于单个 block，
 * 查询时没有必要每次都把整张表全部读出来。
 */
class ObSSTable : public enable_shared_from_this<ObSSTable>
{
public:
  /**
   * @brief 构造一个 SSTable 文件视图对象。
   *
   * 构造函数本身不做 I/O，真正解析文件内容要等 `init()`。
   */
  ObSSTable(uint32_t sst_id, const string &file_name, const ObComparator *comparator,
      ObLRUCache<uint64_t, shared_ptr<ObBlock>> *block_cache)
      : sst_id_(sst_id),
        file_name_(file_name),
        comparator_(comparator),
        file_reader_(nullptr),
        block_cache_(block_cache)
  {
    (void)block_cache_;
  }

  ~ObSSTable() = default;

  /**
   * @brief 初始化 SSTable。
   *
   * 典型工作包括：
   * - 打开文件；
   * - 读取尾部 meta；
   * - 恢复所有 block 的位置信息。
   *
   * init 之后，SSTable 对象就变成一个“只读文件视图”：
   * - block 内容按需读；
   * - block meta 常驻内存；
   * - 多个 TableIterator 可以共享同一个 SSTable 实例。
   */
  void init();

  uint32_t sst_id() const { return sst_id_; }

  shared_ptr<ObSSTable> get_shared_ptr() { return shared_from_this(); }

  ObLsmIterator *new_iterator();

  /**
   * @brief 优先通过 block cache 读取指定 block。
   */
  shared_ptr<ObBlock> read_block_with_cache(uint32_t block_idx) const;

  /**
   * @brief 绕过缓存直接从文件读取指定 block。
   */
  shared_ptr<ObBlock> read_block(uint32_t block_idx) const;

  uint32_t block_count() const { return block_metas_.size(); }

  uint32_t size() const { return file_reader_->file_size(); }

  const BlockMeta block_meta(int i) const { return block_metas_[i]; }

  const ObComparator *comparator() const { return comparator_; }

  // 删除 SSTable 对应的物理文件。
  void   remove();
  string first_key() const { return block_metas_.empty() ? "" : block_metas_[0].first_key_; }
  string last_key() const { return block_metas_.empty() ? "" : block_metas_.back().last_key_; }

private:
  uint32_t                 sst_id_;
  string                   file_name_;
  const ObComparator      *comparator_ = nullptr;
  unique_ptr<ObFileReader> file_reader_;
  vector<BlockMeta>        block_metas_;

  ObLRUCache<uint64_t, shared_ptr<ObBlock>> *block_cache_;
};

class TableIterator : public ObLsmIterator
{
public:
  TableIterator(const shared_ptr<ObSSTable> &sst) : sst_(sst), block_cnt_(sst->block_count()) {}
  ~TableIterator() override = default;

  // 表级 iterator = “当前 block 下标” + “当前块内 iterator”。
  // 它只在跨块时重新装载 block，块内移动完全委托给 BlockIterator。
  void        seek(const string_view &key) override;
  void        seek_to_first() override;
  void        seek_to_last() override;
  void        next() override;
  bool        valid() const override { return block_iterator_ != nullptr && block_iterator_->valid(); }
  string_view key() const override { return block_iterator_->key(); }
  string_view value() const override { return block_iterator_->value(); }

private:
  // 根据 curr_block_idx_ 装载当前 block，并构造块内迭代器。
  // 如果 block cache 命中，这一步通常不会触发真实磁盘读取。
  void read_block_with_cache();

  const shared_ptr<ObSSTable> sst_;
  uint32_t                    block_cnt_      = 0;
  uint32_t                    curr_block_idx_ = 0;
  shared_ptr<ObBlock>         block_;
  unique_ptr<ObLsmIterator>   block_iterator_;
};

// 外层 vector 表示 level / run，内层 vector 表示该层上的多个 SSTable。
// 这与 LSM 多层结构对应，后续查询或 compaction 会遍历这个二维集合。
using SSTablesPtr = shared_ptr<vector<vector<shared_ptr<ObSSTable>>>>;

}  // namespace oceanbase
