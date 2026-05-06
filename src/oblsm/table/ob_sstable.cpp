/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "oblsm/table/ob_sstable.h"
#include "oblsm/util/ob_coding.h"
#include "common/log/log.h"
#include "common/lang/filesystem.h"
namespace oceanbase {

void ObSSTable::init()
{
  // TODO: 这里应当负责打开文件并解析 block meta。
  // 当前注释保留初始化职责，方便后续补实现时直接对照。
  // 按预期，初始化顺序通常是：
  // 1. 打开文件并拿到总长度；
  // 2. 从尾部读出 meta 区范围；
  // 3. 逐条 decode BlockMeta，恢复每个 block 的 key 范围与物理位置。
}

shared_ptr<ObBlock> ObSSTable::read_block_with_cache(uint32_t block_idx) const
{
  // TODO: 读取路径的理想顺序应为：
  // 1. 先查 block_cache_；
  // 2. miss 时回源 read_block()；
  // 3. 再把结果回填缓存。
  // block cache 的 key 通常需要把 `sst_id + block_idx` 组合起来，
  // 以避免不同 SSTable 中相同 block 编号发生冲突。
  return nullptr;
}

shared_ptr<ObBlock> ObSSTable::read_block(uint32_t block_idx) const
{
  // TODO: 直接根据 block meta 读取块字节流并 decode 成 ObBlock。
  // 这里是 SSTable 真正的 I/O 边界：
  // - 先通过 `block_metas_[block_idx]` 拿到 offset/size；
  // - 再从文件中读出对应字节；
  // - 最后交给 `ObBlock::decode()` 还原为可迭代的内存对象。
  return nullptr;
}

void ObSSTable::remove() { filesystem::remove(file_name_); }

ObLsmIterator *ObSSTable::new_iterator() { return new TableIterator(get_shared_ptr()); }

void TableIterator::read_block_with_cache()
{
  // TableIterator 只维持“当前块”的 iterator，跨块时再懒加载下一块。
  // 这样内存占用与访问局部性都更可控，不需要一次性展开整张 SSTable。
  block_ = sst_->read_block_with_cache(curr_block_idx_);
  block_iterator_.reset(block_->new_iterator());
}

void TableIterator::seek_to_first()
{
  // 表级最小 key 一定位于第一个 block 的第一条 entry。
  curr_block_idx_ = 0;
  read_block_with_cache();
  block_iterator_->seek_to_first();
}

void TableIterator::seek_to_last()
{
  // 表级最大 key 一定位于最后一个 block 的最后一条 entry。
  curr_block_idx_ = block_cnt_ - 1;
  read_block_with_cache();
  block_iterator_->seek_to_last();
}

void TableIterator::next()
{
  // 先在当前 block 内前进；走到块尾后再切到下一个 block。
  // 这里体现了“两级迭代器”设计：
  // - 块内由 BlockIterator 负责；
  // - 块间切换由 TableIterator 负责。
  block_iterator_->next();
  if (block_iterator_->valid()) {
  } else if (curr_block_idx_ < block_cnt_ - 1) {
    curr_block_idx_++;
    read_block_with_cache();
    block_iterator_->seek_to_first();
  }
}

void TableIterator::seek(const string_view &lookup_key)
{
  curr_block_idx_ = 0;
  // 先根据 block meta 做块级别粗定位，避免从第一个 block 一直扫到最后。
  // 这里只做线性扫描，后续可优化为二分查找。
  // 判定条件使用 block 的 `last_key_`：
  // 只要某个 block 的最大 key 已经不小于目标 user key，
  // 目标就有可能落在该 block 中。
  for (; curr_block_idx_ < block_cnt_; curr_block_idx_++) {
    const auto &block_meta = sst_->block_meta(curr_block_idx_);
    if (sst_->comparator()->compare(extract_user_key(block_meta.last_key_), extract_user_key_from_lookup_key(lookup_key)) >= 0) {
      break;
    }
  }
  if (curr_block_idx_ == block_cnt_) {
    block_iterator_ = nullptr;
    return;
  }
  read_block_with_cache();
  // 块级定位命中后，再交给 BlockIterator 做更细粒度的块内定位。
  block_iterator_->seek(lookup_key);
};

}  // namespace oceanbase
