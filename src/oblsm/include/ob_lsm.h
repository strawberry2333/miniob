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

#include "common/lang/string.h"
#include "common/lang/string_view.h"
#include "common/sys/rc.h"
#include "common/lang/utility.h"
#include "oblsm/include/ob_lsm_options.h"
#include "oblsm/include/ob_lsm_iterator.h"

namespace oceanbase {

class ObLsmTransaction;
/**
 * @brief ObLsm 对外暴露的 LSM-Tree Key-Value 存储接口。
 *
 * 这个类只定义“能力边界”，不关心底层实现细节：
 * - 写入路径通常是 WAL -> MemTable -> SSTable；
 * - 读取路径通常是 MemTable / Immutable MemTable / SSTable 的多路归并；
 * - 恢复路径通常依赖 Manifest 与 WAL。
 *
 * 当前工程中的具体实现类是 `ObLsmImpl`，这里保留抽象接口有两个目的：
 * 1. 让上层调用方只依赖稳定的 API，而不依赖具体实现；
 * 2. 便于后续替换存储引擎实现或补充 Mock/Test 实现。
 *
 * 另外，接口层故意不暴露 Manifest、WAL、SSTable 之类内部对象，
 * 避免调用方绕过调度器直接操作底层状态，破坏恢复链路的一致性。
 */
class ObLsm
{
public:
  /**
   * @brief 打开指定目录下的 ObLsm 实例。
   *
   * 这是模块的统一入口。实现上会：
   * 1. 创建 `ObLsmImpl` 对象；
   * 2. 打开/恢复 Manifest；
   * 3. 根据 Manifest 与 WAL 恢复内存状态和磁盘状态；
   * 4. 将最终实例返回给调用方。
   *
   * @param options LSM 引擎配置项，例如 memtable 大小、compaction 策略等。
   * @param path 数据目录。SST、Manifest、WAL 都会落在该目录下。
   * @param dbptr 输出参数，返回堆上分配出的数据库对象。
   * @return 成功返回 `RC::SUCCESS`，失败返回对应错误码。
   * @note 返回对象由调用方负责释放。
   */
  static RC open(const ObLsmOptions &options, const string &path, ObLsm **dbptr);

  ObLsm() = default;

  ObLsm(const ObLsm &) = delete;

  ObLsm &operator=(const ObLsm &) = delete;

  virtual ~ObLsm() = default;

  /**
   * @brief 写入一条键值对。
   *
   * 在 LSM 语义里，“更新”本质上也是一次新的追加写入：
   * 同一个 user key 会携带更大的序列号写入，读取时由可见性规则选出最新版本。
   * 因此旧版本通常仍会暂时留在磁盘或内存结构中，直到后续 compaction 才可能被真正清理。
   *
   * @param key 用户键。
   * @param value 用户值。
   * @return 成功返回 `RC::SUCCESS`。
   */
  virtual RC put(const string_view &key, const string_view &value) = 0;

  /**
   * @brief 读取指定 key 对应的最新可见值。
   *
   * 实现通常会构造统一迭代器，把内存和磁盘上的多份数据视为一个有序流，
   * 再根据内部 key 中的序列号与删除标记决定最终返回值。
   *
   * @param key 待查找的用户键。
   * @param value 输出参数，命中后写入结果值。
   * @return 找到返回 `RC::SUCCESS`，不存在通常返回 `RC::NOT_EXIST`。
   */
  virtual RC get(const string_view &key, string *value) = 0;

  /**
   * @brief 删除一条键值记录。
   *
   * 在 LSM 中，删除通常不是立刻物理移除，而是写入一个 value 为空的 tombstone。
   * 后续 compaction 才有机会真正丢弃被覆盖的数据版本。
   *
   * @param key 待删除的用户键。
   * @return 成功返回 `RC::SUCCESS`。
   */
  virtual RC remove(const string_view &key) = 0;

  // TODO: distinguish transaction interface and non-transaction interface, refer to rocksdb
  /**
   * @brief 开启一个事务对象。
   *
   * 目前事务能力仍在演进中，接口已经预留，但大部分实现尚未完成。
   */
  virtual ObLsmTransaction *begin_transaction() = 0;

  /**
   * @brief 创建遍历数据库内容的迭代器。
   *
   * 返回的通常是“用户视角”的迭代器，而不是直接暴露内部 key。
   * 也就是说，调用方看到的是：
   * - 已经按 user key 去重后的结果；
   * - 已经根据 seq 过滤过版本后的结果；
   * - 已经屏蔽 tombstone 的结果。
   *
   * @param options 读选项，例如指定可见版本上限。
   * `options.seq == -1` 时表示读当前最新视图，否则表示构造一个“历史快照读”。
   * @return 新建的迭代器对象，生命周期由调用方管理。
   */
  virtual ObLsmIterator *new_iterator(ObLsmReadOptions options) = 0;

  /**
   * @brief 批量写入多条键值对。
   *
   * 当前接口已预留，但底层实现还未完整支持。
   *
   * @param kvs 待写入的键值对集合。
   * @return 成功返回 `RC::SUCCESS`，未实现时可能返回 `RC::UNIMPLEMENTED`。
   */
  virtual RC batch_put(const vector<pair<string, string>> &kvs) = 0;

  /**
   * @brief 输出当前 SSTable 分层/分组信息，主要用于调试。
   *
   * 这里暴露的是引擎内部布局信息，而不是用户视角的数据结果，适合排查 compaction 是否按预期工作。
   */
  virtual void dump_sstables() = 0;
};

}  // namespace oceanbase
