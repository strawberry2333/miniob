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
#include "common/lang/unordered_map.h"
#include "common/lang/mutex.h"
#include "catalog/table_stats.h"

/**
 * @brief observer 侧的轻量级元数据目录。
 * @details 当前 catalog 只缓存表统计信息，主要供优化器和执行层读取；
 * 数据只驻留在进程内存中，生命周期与 observer 进程一致。
 */

/**
 * @class Catalog
 * @brief Store metadata, such as table statistics.
 *
 * The Catalog class provides methods to access and update metadata related to
 * tables, such as table statistics. It is designed as a singleton to ensure
 * a single point of management for the metadata.
 */
class Catalog
{

public:
  /// @brief 构造一个空目录，实际使用时通常通过 get_instance() 访问单例。
  Catalog() = default;

  /// @brief 默认析构，不负责持久化当前缓存。
  ~Catalog() = default;

  Catalog(const Catalog &) = delete;

  Catalog &operator=(const Catalog &) = delete;

  /**
   * @brief Retrieves table statistics for a given table_id.
   *
   * @param table_id The identifier of the table for which statistics are requested.
   * @return A constant reference to the TableStats of the specified table_id.
   * @note 内部使用 `unordered_map::operator[]`，不存在的表会先插入一个默认统计项。
   */
  const TableStats &get_table_stats(int table_id);

  /**
   * @brief Updates table statistics for a given table.
   *
   * @param table_id The identifier of the table for which statistics are updated.
   * @param table_stats The new table statistics to be set.
   * @note 调用方需要保证缓存中的统计与真实表状态同步更新。
   */
  void update_table_stats(int table_id, const TableStats &table_stats);

  /**
   * @brief Gets the singleton instance of the Catalog.
   *
   * This method ensures there is only one instance of the Catalog.
   *
   * @return A reference to the singleton instance of the Catalog.
   * @note 单例实例由函数内静态对象托管，首次访问时初始化。
   */
  static Catalog &get_instance()
  {
    static Catalog instance;
    return instance;
  }

private:
  mutex mutex_;

  /**
   * @brief A map storing the table statistics indexed by table_id.
   *
   * This map is currently not persisted and its persistence is planned via a system table.
   */
  unordered_map<int, TableStats> table_stats_;  ///< Table statistics storage.
};
