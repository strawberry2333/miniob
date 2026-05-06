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

#include <cstddef>
#include <cstdint>
#include "oblsm/ob_lsm_define.h"

namespace oceanbase {

/**
 * @brief ObLsm 的运行参数。
 *
 * 这些参数同时影响：
 * - 写入放大：例如 memtable 多大时冻结；
 * - 读放大：例如 leveled compaction 的层数和每层规模；
 * - 空间放大：例如 tiered/leveled 不同策略下保留多少冗余数据；
 * - 崩溃恢复开销：例如 WAL 是否每次写都强制 fsync。
 */
struct ObLsmOptions
{
  ObLsmOptions(){};

  // 当前可变 MemTable 的近似上限。超过后会触发 freeze，转成 immutable memtable。
  size_t memtable_size = 8 * 1024;
  // 单个 SSTable 的目标大小，major compaction 输出文件时会参考它进行切分。
  size_t table_size = 16 * 1024;

  // leveled compaction 的默认层数。
  size_t default_levels        = 7;
  // L1 的目标容量，后续层通常按比例扩张。
  size_t default_l1_level_size = 128 * 1024;
  // 相邻层目标容量倍率。
  size_t default_level_ratio   = 10;
  // L0 文件数阈值，常用于触发 leveled compaction。
  size_t default_l0_file_num   = 3;

  // tiered（代码里命名为 tired）compaction 的 run 数阈值。
  size_t default_run_num = 7;

  // 当前选择的 compaction 策略。
  CompactionType type = CompactionType::LEVELED;

  // 是否每次新写入都强制同步 WAL 到磁盘。
  // true: 崩溃恢复更安全，但写放大和延迟更高。
  // false: 依赖后续批量 sync，吞吐更高但可能丢失最近一小段日志。
  bool force_sync_new_log = true;
};

/**
 * @brief 读选项。
 *
 * 当前最关键的参数是 `seq`：
 * - `-1` 表示读取当前最新可见版本；
 * - 其他值表示构造一个“历史快照视图”，只看 `<= seq` 的版本。
 */
struct ObLsmReadOptions
{
  ObLsmReadOptions(){};

  int64_t seq = -1;
};

}  // namespace oceanbase
