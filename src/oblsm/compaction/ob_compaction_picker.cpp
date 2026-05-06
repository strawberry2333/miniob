/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "oblsm/compaction/ob_compaction_picker.h"
#include "common/log/log.h"

namespace oceanbase {

// 当前的 tiered 选表逻辑只有一个简单阈值：
// 当 run/SSTable 数量还没达到 `default_run_num` 时，不值得做一次全量 merge；
// 一旦达到阈值，就把当前所有输入都并到同一次 compaction 中。
unique_ptr<ObCompaction> TiredCompactionPicker::pick(SSTablesPtr sstables)
{
  // 不满足触发条件时直接返回空，表示本轮无需 compaction。
  if (sstables->size() < options_->default_run_num) {
    return nullptr;
  }
  unique_ptr<ObCompaction> compaction(new ObCompaction(0));
  // 这里没有做更细的“挑部分 run”或“按 key range 过滤重叠”的优化，
  // 只要达到阈值，就把当前所有 SSTable 都纳入 `inputs_[0]`。
  // 这意味着 picker 输出的是一个粗粒度任务：实现简单，但可能带来更高写放大。
  for (size_t i = 0; i < sstables->size(); ++i) {
    size_t tire_i_size = (*sstables)[i].size();
    for (size_t j = 0; j < tire_i_size; ++j) {
      compaction->inputs_[0].emplace_back((*sstables)[i][j]);
    }
  }
  // 对 tiered 模式而言，当前没有第二组重叠输入，因此 `inputs_[1]` 为空。
  return compaction;
}

ObCompactionPicker *ObCompactionPicker::create(CompactionType type, ObLsmOptions *options)
{
  // 恢复或初始化时会通过这个分发点拿到与配置一致的 picker 实现。
  switch (type) {
    case CompactionType::TIRED: return new TiredCompactionPicker(options);
    default: return nullptr;
  }
  return nullptr;
}

}  // namespace oceanbase
