/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "catalog/catalog.h"

/**
 * @brief Catalog 的实现文件。
 * @details 所有访问都通过同一个互斥锁保护，保证多线程环境下对统计缓存的读写安全。
 */

/**
 * @brief 查询表统计信息。
 * @param table_id 目标表的内部编号。
 * @return 对应统计对象的只读引用；若不存在则返回一个默认构造出来的统计对象。
 */
const TableStats &Catalog::get_table_stats(int table_id)
{
  lock_guard<mutex> lock(mutex_);
  // 使用 operator[] 合并“查找”和“首次创建默认项”这两条路径。
  return table_stats_[table_id];
}

/**
 * @brief 覆盖更新某张表的统计信息。
 * @param table_id 目标表的内部编号。
 * @param table_stats 新的统计快照。
 */
void Catalog::update_table_stats(int table_id, const TableStats &table_stats)
{
  lock_guard<mutex> lock(mutex_);
  table_stats_[table_id] = table_stats;
}
