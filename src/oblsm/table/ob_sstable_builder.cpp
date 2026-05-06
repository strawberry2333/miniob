/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "oblsm/table/ob_sstable_builder.h"
#include "oblsm/util/ob_coding.h"

namespace oceanbase {

// TODO: 这里未来需要把“遍历 MemTable -> 切块 -> 写 meta”的完整流程补齐。
// 当前 build 还未实现，但 finish_build_block/get_built_table 已体现了 SSTable 的组织方式。
RC ObSSTableBuilder::build(shared_ptr<ObMemTable> mem_table, const std::string &file_name, uint32_t sst_id)
{
  // 预期的完整流程应当是：
  // 1. reset 当前 builder 状态并创建文件写句柄；
  // 2. 通过 MemTableIterator 顺序扫描 internal key/value；
  // 3. 调用 block_builder_.add()，满块时 finish_build_block()；
  // 4. 所有数据块写完后，再把 block_metas_ 序列化追加到文件尾部；
  // 5. 记录 file_size_，返回可重新打开该文件的 ObSSTable。
  //
  // 这里仍保持未实现，只补充注释说明设计意图，不改行为。
  return RC::UNIMPLEMENTED;
}

void ObSSTableBuilder::finish_build_block()
{
  // 当前 block 完成后：
  // 1. 把 block 原始内容写入 SSTable；
  // 2. 记录首尾 key 和块位置，供后续读路径利用。
  // 这一步是“写数据”和“写索引”之间的桥：
  // 真正的数据先顺序落盘，而块级索引信息则缓存在 block_metas_ 中，等待文件尾统一写出。
  string      last_key       = block_builder_.last_key();
  string_view block_contents = block_builder_.finish();
  file_writer_->write(block_contents);
  block_metas_.push_back(BlockMeta(curr_blk_first_key_, last_key, curr_offset_, block_contents.size()));
  // 当前实现直接顺序拼接 block，暂未做页对齐。
  curr_offset_ += block_contents.size();
  block_builder_.reset();
}

shared_ptr<ObSSTable> ObSSTableBuilder::get_built_table()
{
  // SSTable 对象会重新从磁盘文件中加载 meta，因此这里返回的是“文件视图对象”，
  // 而不是把内存中的 block/meta 直接塞进去。
  // 这样读路径和写路径共享同一套解码逻辑，避免“内存态”和“磁盘态”出现两套语义。
  shared_ptr<ObSSTable> sstable = make_shared<ObSSTable>(sst_id_, file_writer_->file_name(), comparator_, block_cache_);
  sstable->init();
  return sstable;
}

void ObSSTableBuilder::reset()
{
  // builder 被设计成可复用对象：
  // 每次 build 前清空块状态、文件句柄、meta 列表和偏移计数器即可。
  block_builder_.reset();
  curr_blk_first_key_.clear();
  if (file_writer_ != nullptr) {
    file_writer_.reset(nullptr);
  }
  block_metas_.clear();
  curr_offset_ = 0;
  sst_id_      = 0;
  file_size_   = 0;
}
}  // namespace oceanbase
