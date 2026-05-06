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
#include "common/sys/rc.h"
#include "oblsm/compaction/ob_compaction.h"
#include "oblsm/util/ob_comparator.h"

namespace oceanbase {

/**
 * @class ObCompactionPicker
 * @brief compaction 选表器抽象基类。
 *
 * picker 的职责是“从当前 SSTable 布局中挑出一项值得执行的 compaction 任务”，
 * 而不是直接执行合并。不同策略的核心差异也主要体现在这里：
 * tiered 更关注 run 数量，leveled 更关注层级大小与重叠范围。
 */
class ObCompactionPicker
{
public:
  /**
   * @param options LSM 配置，picker 会依据其中的阈值决定何时触发 compaction。
   */
  ObCompactionPicker(ObLsmOptions *options) : options_(options) {}

  virtual ~ObCompactionPicker() = default;

  /**
   * @brief 从当前 SSTable 视图中挑选一项 compaction。
   *
   * 返回 `nullptr` 表示此刻不需要 compaction；调用方可继续接受写入或等待下次检查。
   */
  virtual unique_ptr<ObCompaction> pick(SSTablesPtr sstables) = 0;

  /**
   * @brief 根据配置类型构造对应的 picker。
   *
   * 工厂函数把“策略枚举 -> 具体实现类”的映射收敛在一处，
   * 便于恢复时根据 Manifest 中记录的 compaction 模式重建相同行为。
   */
  static ObCompactionPicker *create(CompactionType type, ObLsmOptions *options);

protected:
  // picker 仅读取配置，不拥有其生命周期。
  ObLsmOptions *options_;
};

/**
 * @class TiredCompactionPicker
 * @brief tiered compaction 的选表器。
 *
 * 当前实现很直接：只要 run 数量达到阈值，就把现有 SSTable 全部作为一批输入。
 * 这不是最省写放大的策略，但实现简单，便于先打通 compaction 主链路。
 */
class TiredCompactionPicker : public ObCompactionPicker
{
public:
  /**
   * @param options LSM 配置，主要使用其中的 `default_run_num` 作为触发阈值。
   */
  TiredCompactionPicker(ObLsmOptions *options) : ObCompactionPicker(options) {}

  ~TiredCompactionPicker() = default;

  /**
   * @brief 按 tiered 规则挑选一项 compaction。
   */
  unique_ptr<ObCompaction> pick(SSTablesPtr sstables) override;

private:
};

}  // namespace oceanbase
