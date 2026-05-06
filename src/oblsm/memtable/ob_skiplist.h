/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#pragma once

// 线程安全说明
// -------------
//
// 这个跳表实现沿用了 LevelDB 的核心思路：
// - 写入需要外部同步，通常由上层 mutex 保证；
// - 读取本身不加内部锁，只要求读期间跳表对象本身不被销毁。
//
// 关键不变量：
// 1. 节点一旦分配，在整个跳表销毁前都不会被释放；
// 2. 节点一旦挂入链表，除了 next 指针外，其余内容都视为不可变；
// 3. 发布节点时通过原子读写保证读线程看到的是“已完整初始化”的节点。
//
// 这种设计非常适合 MemTable：
// - 插入很多；
// - 删除几乎没有（冻结后整体丢弃）；
// - 需要始终保持有序以支持后续 flush/迭代。

#include "common/math/random_generator.h"
#include "common/lang/atomic.h"
#include "common/lang/vector.h"
#include "common/log/log.h"

namespace oceanbase {

template <typename Key, class ObComparator>
class ObSkipList
{
private:
  struct Node;

public:
  /**
   * @brief 创建一个使用给定比较器的跳表。
   *
   * 跳表中的全局有序性完全由 `ObComparator` 决定。
   * 对 MemTable 而言，这个比较器通常会把底层字节解释成 internal key，
   * 从而把“用户键顺序 + 版本顺序”一起编码进跳表结构。
   */
  explicit ObSkipList(ObComparator cmp);

  ObSkipList(const ObSkipList &)            = delete;
  ObSkipList &operator=(const ObSkipList &) = delete;
  ~ObSkipList();

  /**
   * @brief 将一个 key 插入跳表。
   * @note 要求当前表中不存在“比较结果相等”的旧 key。
   *
   * 这里的“相等”指比较器语义上的相等，而不是对象地址相等。
   * 如果比较器认为两个节点拥有同一排序键，再插入就会破坏有序不变量。
   */
  void insert(const Key &key);

  // 并发插入版本与 `insert()` 语义一致，只是通过 CAS 逐层发布新节点。
  void insert_concurrently(const Key &key);

  /**
   * @brief 判断跳表中是否存在与给定 key 相等的元素。
   */
  bool contains(const Key &key) const;

  /**
   * @brief 跳表只读迭代器。
   *
   * 语义上它提供的是“有序单链表 + 多层索引”的只读游标：
   * - `next()` 沿最底层链表顺序前进；
   * - `seek()` 借助高层索引快速跳到目标附近；
   * - `prev()` 没有反向指针，因此需要重新搜索前驱。
   */
  class Iterator
  {
  public:
    /**
     * @brief 基于给定跳表构造迭代器。
     * @note 构造完成后默认无效，需要显式 seek。
     */
    explicit Iterator(const ObSkipList *list);

    /**
     * @brief 当前是否指向有效节点。
     */
    bool valid() const;

    /**
     * @brief 返回当前节点的 key。
     */
    const Key &key() const;

    /**
     * @brief 前进到下一个节点。
     */
    void next();

    /**
     * @brief 回退到前一个节点。
     *
     * 跳表并不维护显式前驱指针，因此该操作不是 O(1)，
     * 而是重新执行一次“找严格前驱”的查找过程。
     */
    void prev();

    /**
     * @brief 定位到第一个 `>= target` 的节点。
     */
    void seek(const Key &target);

    /**
     * @brief 定位到第一个节点。
     */
    void seek_to_first();

    /**
     * @brief 定位到最后一个节点。
     */
    void seek_to_last();

  private:
    const ObSkipList *list_;
    Node             *node_;
  };

private:
  enum
  {
    // 最大塔高。层数越大，平均查找更快，但每个节点需要维护更多 next 指针。
    kMaxHeight = 12
  };

  inline int get_max_height() const { return max_height_.load(std::memory_order_relaxed); }

  Node *new_node(const Key &key, int height);
  int   random_height();
  bool  equal(const Key &a, const Key &b) const { return (compare_(a, b) == 0); }

  // 找到第一个 `>= key` 的节点。
  // 若 prev 非空，还会顺手记录每一层上的前驱节点，供插入新节点时使用。
  // 这是跳表查找/插入共享的核心下降流程：从最高层开始横向扫描，再逐层下探。
  Node *find_greater_or_equal(const Key &key, Node **prev) const;

  // 找到最后一个 `< key` 的节点；若不存在则返回头节点。
  Node *find_less_than(const Key &key) const;

  // 找到整张表的最后一个节点；空表时返回头节点。
  Node *find_last() const;

  // 构造后不再替换的比较器实例。
  ObComparator const compare_;

  // 哨兵头节点，不承载真实业务 key，只作为每层链表的统一入口。
  Node *const head_;

  // 当前整张跳表的最高层数。只有写线程更新，读线程看到旧值也不影响正确性。
  // 原因是：读线程最多少走几层索引，性能可能略差，但不会破坏“最终落到正确位置”。
  atomic<int> max_height_;

  // 共享随机数发生器，用于给新节点分配层高。
  static common::RandomGenerator rnd;
};

template <typename Key, class ObComparator>
common::RandomGenerator ObSkipList<Key, ObComparator>::rnd = common::RandomGenerator();

// 以下是实现细节。
template <typename Key, class ObComparator>
struct ObSkipList<Key, ObComparator>::Node
{
  explicit Node(const Key &k) : key(k) {}

  // 节点 key 在发布后不再修改，便于无锁读线程安全观察。
  Key const key;

  // next 指针统一封装在方法里，便于精确控制原子内存序。
  // 对 MemTable 来说，节点一旦发布就不会被删除，因此只要保证“先初始化节点内容，
  // 再把 next 指针发布出去”，无锁读线程就能安全遍历。
  Node *next(int n)
  {
    ASSERT(n >= 0, "n >= 0");
    // acquire load 保证读线程看到的节点内容已经完整初始化。
    return next_[n].load(std::memory_order_acquire);
  }
  void set_next(int n, Node *x)
  {
    ASSERT(n >= 0, "n >= 0");
    // release store 负责“发布”节点。
    next_[n].store(x, std::memory_order_release);
  }

  // 在局部已知安全的场景下使用 relaxed 变体，减少额外同步成本。
  // 典型场景是写线程在尚未把节点暴露给其他线程之前，先搭好新节点的多层 next 链。
  Node *nobarrier_next(int n)
  {
    ASSERT(n >= 0, "n >= 0");
    return next_[n].load(std::memory_order_relaxed);
  }
  void nobarrier_set_next(int n, Node *x)
  {
    ASSERT(n >= 0, "n >= 0");
    next_[n].store(x, std::memory_order_relaxed);
  }

  bool cas_next(int n, Node *expected, Node *x)
  {
    ASSERT(n >= 0, "n >= 0");
    return next_[n].compare_exchange_strong(expected, x);
  }

private:
  // 变长数组，实际长度等于节点高度。`next_[0]` 是最底层、也是完整有序链表所在层，
  // 更高层仅作为“稀疏索引”加速查找。
  atomic<Node *> next_[1];
};

template <typename Key, class ObComparator>
typename ObSkipList<Key, ObComparator>::Node *ObSkipList<Key, ObComparator>::new_node(const Key &key, int height)
{
  // 为变高节点一次性申请足够空间，避免 next 指针分散分配。
  char *const node_memory = reinterpret_cast<char *>(malloc(sizeof(Node) + sizeof(atomic<Node *>) * (height - 1)));
  return new (node_memory) Node(key);
}

template <typename Key, class ObComparator>
inline ObSkipList<Key, ObComparator>::Iterator::Iterator(const ObSkipList *list)
{
  list_ = list;
  node_ = nullptr;
}

template <typename Key, class ObComparator>
inline bool ObSkipList<Key, ObComparator>::Iterator::valid() const
{
  return node_ != nullptr;
}

template <typename Key, class ObComparator>
inline const Key &ObSkipList<Key, ObComparator>::Iterator::key() const
{
  ASSERT(valid(), "valid");
  return node_->key;
}

template <typename Key, class ObComparator>
inline void ObSkipList<Key, ObComparator>::Iterator::next()
{
  ASSERT(valid(), "valid");
  node_ = node_->next(0);
}

template <typename Key, class ObComparator>
inline void ObSkipList<Key, ObComparator>::Iterator::prev()
{
  // 没有显式 prev 链，只能回头搜索“严格小于当前 key 的最后一个节点”。
  ASSERT(valid(), "valid");
  node_ = list_->find_less_than(node_->key);
  if (node_ == list_->head_) {
    node_ = nullptr;
  }
}

template <typename Key, class ObComparator>
inline void ObSkipList<Key, ObComparator>::Iterator::seek(const Key &target)
{
  node_ = list_->find_greater_or_equal(target, nullptr);
}

template <typename Key, class ObComparator>
inline void ObSkipList<Key, ObComparator>::Iterator::seek_to_first()
{
  node_ = list_->head_->next(0);
}

template <typename Key, class ObComparator>
inline void ObSkipList<Key, ObComparator>::Iterator::seek_to_last()
{
  node_ = list_->find_last();
  if (node_ == list_->head_) {
    node_ = nullptr;
  }
}

template <typename Key, class ObComparator>
int ObSkipList<Key, ObComparator>::random_height()
{
  // 层高采用几何分布：
  // - 大部分节点只在低层出现；
  // - 少数节点会出现在高层，充当“路标”；
  // 从而把查找复杂度维持在期望 O(log N)。
  static const unsigned int kBranching = 4;
  int                       height     = 1;
  while (height < kMaxHeight && rnd.next(kBranching) == 0) {
    height++;
  }
  ASSERT(height > 0, "height > 0");
  ASSERT(height <= kMaxHeight, "height <= kMaxHeight");
  return height;
}

template <typename Key, class ObComparator>
typename ObSkipList<Key, ObComparator>::Node *ObSkipList<Key, ObComparator>::find_greater_or_equal(
    const Key &key, Node **prev) const
{
  // 预期算法：
  // 1. 从最高层开始扫描；
  // 2. 当前层若还能向右且右侧 key < 目标 key，则继续前进；
  // 3. 否则记录本层前驱并下降一层；
  // 4. 到第 0 层后返回第一个 `>= key` 的节点。
  //
  // `prev` 中留下的是每层最后一个 `< key` 的节点，插入新节点时会直接复用。
  // 当前仓库尚未把这段核心查找逻辑补齐，因此先返回空指针占位。
  return nullptr;
}

template <typename Key, class ObComparator>
typename ObSkipList<Key, ObComparator>::Node *ObSkipList<Key, ObComparator>::find_less_than(const Key &key) const
{
  Node *x     = head_;
  int   level = get_max_height() - 1;
  while (true) {
    ASSERT(x == head_ || compare_(x->key, key) < 0, "x == head_ || compare_(x->key, key) < 0");
    Node *next = x->next(level);
    if (next == nullptr || compare_(next->key, key) >= 0) {
      if (level == 0) {
        return x;
      } else {
        // 当前层已无法再向前，下降到更细粒度的一层继续查找。
        level--;
      }
    } else {
      x = next;
    }
  }
}

template <typename Key, class ObComparator>
typename ObSkipList<Key, ObComparator>::Node *ObSkipList<Key, ObComparator>::find_last() const
{
  Node *x     = head_;
  int   level = get_max_height() - 1;
  while (true) {
    Node *next = x->next(level);
    if (next == nullptr) {
      if (level == 0) {
        return x;
      } else {
        // 当前层已到末尾，下降一层继续尝试。
        level--;
      }
    } else {
      x = next;
    }
  }
}

template <typename Key, class ObComparator>
ObSkipList<Key, ObComparator>::ObSkipList(ObComparator cmp)
    : compare_(cmp), head_(new_node(0 /* any key will do */, kMaxHeight)), max_height_(1)
{
  // 头节点默认拥有满高塔，便于所有层共用同一个起点。
  for (int i = 0; i < kMaxHeight; i++) {
    head_->set_next(i, nullptr);
  }
}

template <typename Key, class ObComparator>
ObSkipList<Key, ObComparator>::~ObSkipList()
{
  typename std::vector<Node *> nodes;
  // 每个节点都会出现在第 0 层，因此沿着最低层就能完整回收整张表。
  nodes.reserve(max_height_.load(std::memory_order_relaxed));
  for (Node *x = head_; x != nullptr; x = x->next(0)) {
    nodes.push_back(x);
  }
  for (auto node : nodes) {
    node->~Node();
    free(node);
  }
}

template <typename Key, class ObComparator>
void ObSkipList<Key, ObComparator>::insert(const Key &key)
{
  // 预期语义：
  // - 先通过 `find_greater_or_equal()` 找到每层前驱；
  // - 给新节点随机生成高度；
  // - 若新高度超过当前 `max_height_`，则更高层前驱默认为 `head_`；
  // - 最后从低层到高层把新节点接入各层链表。
}

template <typename Key, class ObComparator>
void ObSkipList<Key, ObComparator>::insert_concurrently(const Key &key)
{
  // 并发版本通常会在每一层利用 CAS 发布 next 指针，
  // 以确保多个写线程竞争插入时，链表结构仍保持有序且可见性正确。
  // 当前实现尚未补齐真正的并发插入流程，这里保留接口占位。
}

template <typename Key, class ObComparator>
bool ObSkipList<Key, ObComparator>::contains(const Key &key) const
{
  Node *x = find_greater_or_equal(key, nullptr);
  if (x != nullptr && equal(key, x->key)) {
    return true;
  } else {
    return false;
  }
}

}  // namespace oceanbase
