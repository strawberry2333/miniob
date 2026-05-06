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

#include "common/sys/rc.h"
#include "common/lang/string.h"
#include "common/lang/string_view.h"
#include "common/lang/memory.h"
#include "oblsm/memtable/ob_skiplist.h"
#include "oblsm/util/ob_comparator.h"
#include "oblsm/util/ob_arena.h"
#include "oblsm/include/ob_lsm_iterator.h"

namespace oceanbase {

/**
 * @class ObMemTable
 * @brief LSM-Tree 的内存写缓冲区。
 *
 * ObMemTable 承担两个职责：
 * 1. 接住最新写入，避免每次写都直接落盘；
 * 2. 以有序结构保存 internal key，方便后续冻结成 SSTable。
 *
 * 当前实现使用跳表作为底层索引结构，跳表节点里保存的是一段自描述编码后的 entry：
 * `| internal_key_size | user_key | seq | value_size | value |`
 *
 * 这样设计的好处是：
 * - 插入时一次性把 key/value 编码到连续内存里；
 * - 比较时只需要拿到长度前缀后的 internal key；
 * - 迭代器可以基于同一段内存零拷贝地返回 key/value 视图。
 */
class ObMemTable : public enable_shared_from_this<ObMemTable>
{
public:
  ObMemTable() : comparator_(), table_(comparator_){};

  ~ObMemTable() = default;

  /**
   * @brief 取出当前对象对应的 `shared_ptr`。
   *
   * 迭代器生命周期可能长于当前调用栈，因此需要通过 `shared_from_this`
   * 保证迭代器持有期间 MemTable 不会被提前释放。
   */
  shared_ptr<ObMemTable> get_shared_ptr() { return shared_from_this(); }

  /**
   * @brief 向 MemTable 追加一条新版本记录。
   *
   * 注意这里不是“原地更新”，而是把 `(user_key, seq, value)` 编码成一条新的 entry
   * 插入跳表。读取时再通过 seq 规则筛选出可见版本。
   */
  void put(uint64_t seq, const string_view &key, const string_view &value);

  /**
   * @brief 返回 MemTable 的近似内存占用。
   *
   * 这里主要依赖 arena 的内存统计，作为是否触发 freeze 的依据。
   */
  size_t appro_memory_usage() const { return arena_.memory_usage(); }

  /**
   * @brief 创建 MemTable 迭代器。
   *
   * 返回的迭代器遍历的是内部编码后的有序 entry，输出 key 为 internal key。
   */
  ObLsmIterator *new_iterator();

private:
  friend class ObMemTableIterator;
  /**
   * @brief 跳表使用的 entry 比较器。
   *
   * 跳表里存的是完整 entry 的起始地址，而不是单独的 key。
   * 因此比较时需要先从 entry 中解出长度前缀后的 internal key，再交给
   * `ObInternalKeyComparator` 处理。
   */
  struct KeyComparator
  {
    const ObInternalKeyComparator comparator;
    explicit KeyComparator() {}
    int operator()(const char *a, const char *b) const;
  };

  // 当前实现采用跳表，是因为它天然支持有序插入和顺序遍历。
  typedef ObSkipList<const char *, KeyComparator> Table;

  /**
   * @brief 决定 MemTable 内部有序性的比较器。
   */
  KeyComparator comparator_;

  /**
   * @brief 真实承载 entry 的有序索引结构。
   */
  Table table_;

  /**
   * @brief MemTable 的内存池。
   *
   * 所有 entry 一次性从 arena 分配，直到整个 MemTable 冻结/销毁时再统一回收，
   * 这样可以避免细粒度 delete 带来的复杂性和碎片化。
   */
  ObArena arena_;
};

/**
 * @class ObMemTableIterator
 * @brief MemTable 的只读遍历器。
 *
 * 它直接遍历跳表中的 entry 地址，再按编码格式把 key/value 切出来。
 */
class ObMemTableIterator : public ObLsmIterator
{
public:
  explicit ObMemTableIterator(shared_ptr<ObMemTable> mem, ObMemTable::Table *table) : mem_(mem), iter_(table) {}

  ObMemTableIterator(const ObMemTableIterator &)            = delete;
  ObMemTableIterator &operator=(const ObMemTableIterator &) = delete;

  ~ObMemTableIterator() override = default;

  void seek(const string_view &k) override;
  void seek_to_first() override { iter_.seek_to_first(); }
  void seek_to_last() override { iter_.seek_to_last(); }

  bool        valid() const override { return iter_.valid(); }
  void        next() override { iter_.next(); }
  string_view key() const override;
  string_view value() const override;

private:
  shared_ptr<ObMemTable>      mem_;
  ObMemTable::Table::Iterator iter_;
  // 预留给 seek 时构造临时 key；当前实现尚未完整利用。
  string tmp_;
};

}  // namespace oceanbase
