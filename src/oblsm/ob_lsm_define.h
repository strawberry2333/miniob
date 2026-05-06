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
namespace oceanbase {

// 各类持久化文件在磁盘上的后缀约定。
static constexpr const char *SSTABLE_SUFFIX  = ".sst";
static constexpr const char *MANIFEST_SUFFIX = ".mf";
static constexpr const char *WAL_SUFFIX      = ".wal";

/**
 * @enum CompactionType
 * @brief oblsm 支持的 compaction 策略类型。
 *
 * `TIRED` 实际含义是 Tiered compaction，只是枚举名沿用了当前代码中的拼写。
 * 两种策略的核心区别在于：
 * - Tiered：允许多个 run 并存，周期性批量合并；
 * - Leveled：强调层级有界和 key range 控制，读放大通常更小。
 */
enum class CompactionType
{
  TIRED = 0,
  LEVELED,
  UNKNOWN,
};

}  // namespace oceanbase
