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
}

shared_ptr<ObBlock> ObSSTable::read_block_with_cache(uint32_t block_idx) const
{
  // TODO: 读取路径的理想顺序应为：
  // 1. 先查 block_cache_；
  // 2. miss 时回源 read_block()；
  // 3. 再把结果回填缓存。
  return nullptr;
}

shared_ptr<ObBlock> ObSSTable::read_block(uint32_t block_idx) const
{
  // TODO: 直接根据 block meta 读取块字节流并 decode 成 ObBlock。
  return nullptr;
}

void ObSSTable::remove() { filesystem::remove(file_name_); }

ObLsmIterator *ObSSTable::new_iterator() { return new TableIterator(get_shared_ptr()); }

void TableIterator::read_block_with_cache()
{
  // TableIterator 只维持“当前块”的 iterator，跨块时再懒加载下一块。
  block_ = sst_->read_block_with_cache(curr_block_idx_);
  block_iterator_.reset(block_->new_iterator());
}

void TableIterator::seek_to_first()
{
  curr_block_idx_ = 0;
  read_block_with_cache();
  block_iterator_->seek_to_first();
}

void TableIterator::seek_to_last()
{
  curr_block_idx_ = block_cnt_ - 1;
  read_block_with_cache();
  block_iterator_->seek_to_last();
}

void TableIterator::next()
{
  // 先在当前 block 内前进；走到块尾后再切到下一个 block。
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
  block_iterator_->seek(lookup_key);
};

}  // namespace oceanbase
