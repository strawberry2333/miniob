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
   */
  explicit ObSkipList(ObComparator cmp);

  ObSkipList(const ObSkipList &)            = delete;
  ObSkipList &operator=(const ObSkipList &) = delete;
  ~ObSkipList();

  /**
   * @brief 将一个 key 插入跳表。
   * @note 要求当前表中不存在“比较结果相等”的旧 key。
   */
  void insert(const Key &key);

  void insert_concurrently(const Key &key);

  /**
   * @brief 判断跳表中是否存在与给定 key 相等的元素。
   */
  bool contains(const Key &key) const;

  /**
   * @brief 跳表只读迭代器。
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
    kMaxHeight = 12
  };

  inline int get_max_height() const { return max_height_.load(std::memory_order_relaxed); }

  Node *new_node(const Key &key, int height);
  int   random_height();
  bool  equal(const Key &a, const Key &b) const { return (compare_(a, b) == 0); }

  // 找到第一个 `>= key` 的节点。
  // 若 prev 非空，还会顺手记录每一层上的前驱节点，供插入新节点时使用。
  Node *find_greater_or_equal(const Key &key, Node **prev) const;

  // 找到最后一个 `< key` 的节点；若不存在则返回头节点。
  Node *find_less_than(const Key &key) const;

  // 找到整张表的最后一个节点；空表时返回头节点。
  Node *find_last() const;

  // Immutable after construction
  ObComparator const compare_;

  Node *const head_;

  // 当前整张跳表的最高层数。只有写线程更新，读线程看到旧值也不影响正确性。
  atomic<int> max_height_;

  static common::RandomGenerator rnd;
};

template <typename Key, class ObComparator>
common::RandomGenerator ObSkipList<Key, ObComparator>::rnd = common::RandomGenerator();

// Implementation details follow
template <typename Key, class ObComparator>
struct ObSkipList<Key, ObComparator>::Node
{
  explicit Node(const Key &k) : key(k) {}

  Key const key;

  // next 指针统一封装在方法里，便于精确控制原子内存序。
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
  // Array of length equal to the node height.  next_[0] is lowest level link.
  atomic<Node *> next_[1];
};

template <typename Key, class ObComparator>
typename ObSkipList<Key, ObComparator>::Node *ObSkipList<Key, ObComparator>::new_node(const Key &key, int height)
{
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
  // Instead of using explicit "prev" links, we just search for the
  // last node that falls before key.
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
  // Increase height with probability 1 in kBranching
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
  // your code here
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
        // Switch to next list
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
        // Switch to next list
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
  for (int i = 0; i < kMaxHeight; i++) {
    head_->set_next(i, nullptr);
  }
}

template <typename Key, class ObComparator>
ObSkipList<Key, ObComparator>::~ObSkipList()
{
  typename std::vector<Node *> nodes;
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
{}

template <typename Key, class ObComparator>
void ObSkipList<Key, ObComparator>::insert_concurrently(const Key &key)
{
  // your code here
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
