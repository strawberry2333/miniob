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
 * 其中 internal key = `user_key + seq`：
 * - `user_key` 负责维持用户视角上的字典序；
 * - `seq` 负责把同一 user key 的多个版本区分开；
 * - 比较器会把 `seq` 当作版本号处理，通常会让更新版本排在更前面，
 *   这样读取时沿着有序流扫描即可先遇到“最新可见版本”。
 *
 * 这样设计的好处是：
 * - 插入时一次性把 key/value 编码到连续内存里；
 * - 比较时只需要拿到长度前缀后的 internal key；
 * - 迭代器可以基于同一段内存零拷贝地返回 key/value 视图；
 * - flush 到 SSTable 时可以直接顺序扫描，无需再额外排序或重编码索引键。
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
   *
   * 这里的 `seq` 已经写进 internal key，所以 MemTable 会并存同一 user key 的
   * 多个历史版本；是否屏蔽旧版本由更上层读路径决定，而不是在写入时覆盖。
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
   * 这个迭代器通常作为两类上游算子的输入：
   * - flush：按当前顺序把内容切块写成 SSTable；
   * - 读路径：与其他 MemTable/SSTable 迭代器一起做多路归并。
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
 * 这里不会复制底层字节；返回的 `string_view` 直接指向 arena 中的 entry 内存，
 * 所以前提是迭代期间对应的 MemTable 不能被释放。
 */
class ObMemTableIterator : public ObLsmIterator
{
public:
  explicit ObMemTableIterator(shared_ptr<ObMemTable> mem, ObMemTable::Table *table) : mem_(mem), iter_(table) {}

  ObMemTableIterator(const ObMemTableIterator &)            = delete;
  ObMemTableIterator &operator=(const ObMemTableIterator &) = delete;

  ~ObMemTableIterator() override = default;

  void seek(const string_view &k) override;
  /**
   * `seek_to_first/last` 分别对应整张跳表上的最小/最大 internal key。
   * 因为跳表天然有序，所以这两个操作只需要委托底层 skiplist iterator。
   */
  void seek_to_first() override { iter_.seek_to_first(); }
  void seek_to_last() override { iter_.seek_to_last(); }

  bool        valid() const override { return iter_.valid(); }
  void        next() override { iter_.next(); }
  string_view key() const override;
  string_view value() const override;

private:
  // 通过 shared_ptr 延长底层 arena 生命周期，保证 iter_ 返回的 string_view 始终有效。
  shared_ptr<ObMemTable>      mem_;
  // 真正执行有序遍历的是底层 skiplist iterator。
  ObMemTable::Table::Iterator iter_;
  // 预留给 seek 时构造临时 key；当前实现尚未完整利用。
  string tmp_;
};

}  // namespace oceanbase
