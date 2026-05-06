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

#include "oblsm/table/ob_sstable.h"
#include "oblsm/include/ob_lsm_options.h"

namespace oceanbase {

class ObCompactionPicker;

/**
 * @class ObCompaction
 * @brief 一次 compaction 任务的只读描述对象。
 *
 * 这个类本身不执行 merge，也不包含输出文件信息；
 * 它只表达“选中了哪些输入表、这些输入表属于哪个层级上下文”。
 * 真正的 compaction 执行器可据此创建迭代器、合并记录并生成新 SSTable。
 */
class ObCompaction
{
public:
  /**
   * @brief 允许 picker 直接填充内部输入集合。
   *
   * 这里把 `inputs_` 保持为私有，是为了让外层只能读取选表结果，
   * 不在任务构造完成后随意修改输入集合。
   */
  friend class ObCompactionPicker;
  friend class TiredCompactionPicker;
  friend class LeveledCompactionPicker;

  /**
   * @brief 构造一项以 `level` 为主层级的 compaction 任务。
   *
   * 对于 tiered/leveled 等不同策略，`level_` 的解释会略有不同，
   * 但都表示“本次挑选是围绕哪个层级发起的”。
   */
  explicit ObCompaction(int level) : level_(level) {}

  ~ObCompaction() = default;

  /**
   * @brief 返回本次 compaction 的主层级。
   */
  int level() const { return level_; }

  /**
   * @brief 读取某个输入分组中的第 `i` 张 SSTable。
   *
   * `which` 的语义约定为：
   * - `0`：主输入集合，通常来自 `level_`；
   * - `1`：次输入集合，通常来自 `level_ + 1`。
   *
   * 对当前的 tiered 实现而言，只会填充 `inputs_[0]`。
   */
  shared_ptr<ObSSTable> input(int which, int i) const { return inputs_[which][i]; }

  /**
   * @brief 返回本次任务涉及的总输入表数。
   */
  int size() const { return inputs_[0].size() + inputs_[1].size(); }

  /**
   * @brief 返回指定输入分组的整组 SSTable。
   */
  const vector<shared_ptr<ObSSTable>> &inputs(int which) const { return inputs_[which]; }

private:
  // compaction 的输入按两个逻辑分组保存，而不是按任意数量的 level 展开：
  // - `inputs_[0]`：主输入，表示“本次主动被挑中的那一批表”；
  // - `inputs_[1]`：与主输入发生重叠、需要一起 merge 的补充输入。
  //
  // 这种布局兼容典型的 leveled compaction，也允许 tiered 策略只使用第一组。
  std::vector<shared_ptr<ObSSTable>> inputs_[2];

  /**
   * @brief 本次 compaction 所围绕的主层级编号。
   */
  int level_;
};

}  // namespace oceanbase
