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

#include "common/lang/memory.h"
#include "oblsm/table/ob_block_builder.h"
#include "oblsm/memtable/ob_memtable.h"
#include "common/lang/string.h"
#include "oblsm/util/ob_file_writer.h"
#include "oblsm/table/ob_block.h"
#include "oblsm/table/ob_sstable.h"
#include "oblsm/util/ob_lru_cache.h"

namespace oceanbase {

/**
 * @brief SSTable 构造器。
 *
 * 它负责把有序的 MemTable 迭代结果切分成多个 block，并最终落成一个 SSTable 文件。
 * 生成过程中会同步维护 `BlockMeta`，供后续查找时做块级定位。
 */
class ObSSTableBuilder
{
public:
  ObSSTableBuilder(const ObComparator *comparator, ObLRUCache<uint64_t, shared_ptr<ObBlock>> *block_cache)
      : comparator_(comparator), block_cache_(block_cache)
  {}
  ~ObSSTableBuilder() = default;

  /**
   * @brief 把一个 MemTable 刷成磁盘 SSTable。
   *
   * 输入必须已经按 internal key 有序，这样输出文件天然有序，
   * 后续读路径和 compaction 才能复用有序归并逻辑。
   */
  RC                    build(shared_ptr<ObMemTable> mem_table, const string &file_name, uint32_t sst_id);
  size_t                file_size() const { return file_size_; }
  // 基于已落盘文件返回可直接参与读/compaction 的 SSTable 对象。
  shared_ptr<ObSSTable> get_built_table();
  // 清理构造状态，准备复用 builder。
  void                  reset();

private:
  // 把当前 block_builder_ 中的数据冲刷到文件，并生成一条 BlockMeta。
  void finish_build_block();

  const ObComparator      *comparator_ = nullptr;
  ObBlockBuilder           block_builder_;
  string                   curr_blk_first_key_;
  unique_ptr<ObFileWriter> file_writer_;
  vector<BlockMeta>        block_metas_;
  uint32_t                 curr_offset_ = 0;
  uint32_t                 sst_id_      = 0;
  size_t                   file_size_   = 0;

  ObLRUCache<uint64_t, shared_ptr<ObBlock>> *block_cache_ = nullptr;
};
}  // namespace oceanbase
