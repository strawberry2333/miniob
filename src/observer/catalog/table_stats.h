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

/**
 * @brief 表统计信息的最小载体。
 * @details 当前只记录行数，用于演示 catalog 的基本能力；后续可以继续追加更复杂的统计字段。
 */

/**
 * @class TableStats
 * @brief Represents statistics related to a table.
 *
 * The TableStats class holds statistical information about a table,
 * such as the number of rows it contains.
 * TODO: Add more statistics as needed.
 */
class TableStats
{
public:
  /// @brief 使用指定行数初始化统计对象。
  explicit TableStats(int row_nums) : row_nums(row_nums) {}

  /// @brief 默认构造一个空统计对象。
  TableStats() = default;

  /// @brief 显式拷贝构造，保证其可以作为 map value 安全复制。
  TableStats(const TableStats &other) { row_nums = other.row_nums; }

  /// @brief 拷贝赋值，仅复制当前已经定义的统计字段。
  TableStats &operator=(const TableStats &other)
  {
    row_nums = other.row_nums;
    return *this;
  }

  ~TableStats() = default;

  int row_nums = 0;  ///< 当前缓存的记录条数。
};
