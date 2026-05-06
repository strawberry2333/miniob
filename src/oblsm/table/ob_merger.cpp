/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "oblsm/table/ob_merger.h"
#include "oblsm/include/ob_lsm_iterator.h"
#include "oblsm/util/ob_comparator.h"
#include "oblsm/ob_lsm_define.h"
#include "oblsm/util/ob_coding.h"

namespace oceanbase {

class ObMergingIterator : public ObLsmIterator
{
public:
  ObMergingIterator(const ObComparator *comparator, vector<unique_ptr<ObLsmIterator>> &&children)
      : comparator_(comparator), children_(std::move(children)), current_(nullptr)
  {}

  ~ObMergingIterator() override = default;

  bool valid() const override { return current_ != nullptr; }

  void seek_to_first() override
  {
    // 让每一路子迭代器都回到起点，再选出全局最小 key 的那一路作为 current_。
    for (size_t i = 0; i < children_.size(); i++) {
      children_[i]->seek_to_first();
    }
    find_smallest();
  }

  void seek_to_last() override
  {
    // 反向场景下取全局最大 key 的那一路。
    for (size_t i = 0; i < children_.size(); i++) {
      children_[i]->seek_to_last();
    }
    find_largest();
  }

  void seek(const string_view &target) override
  {
    // 每一路都 seek 到 >= target 的位置，然后从这些候选者里挑出最小者。
    for (size_t i = 0; i < children_.size(); i++) {
      children_[i]->seek(target);
    }
    find_smallest();
  }

  void next() override
  {
    // 只有当前最小的那一路需要前进；前进后再重新选全局最小。
    current_->next();
    find_smallest();
  }

  string_view key() const override { return current_->key(); }

  string_view value() const override { return current_->value(); }

private:
  void find_smallest();
  void find_largest();

  // 当前 children 数量一般很小（MemTable + 少量 SSTable），
  // 所以直接线性扫描选最小值，比维护堆更简单。
  const ObComparator *              comparator_;
  vector<unique_ptr<ObLsmIterator>> children_;
  ObLsmIterator *                   current_;
};

void ObMergingIterator::find_smallest()
{
  ObLsmIterator *smallest = nullptr;
  for (size_t i = 0; i < children_.size(); i++) {
    ObLsmIterator *child = children_[i].get();
    if (child->valid()) {
      if (smallest == nullptr) {
        smallest = child;
      } else if (comparator_->compare(child->key(), smallest->key()) < 0) {
        // 比较器决定了“新版本更靠前”还是“老版本更靠前”等关键顺序语义。
        smallest = child;
      }
    }
  }
  current_ = smallest;
}

void ObMergingIterator::find_largest()
{
  ObLsmIterator *largest = nullptr;
  for (size_t i = 0; i < children_.size(); i++) {
    ObLsmIterator *child = children_[i].get();
    if (child->valid()) {
      if (largest == nullptr) {
        largest = child;
      } else if (comparator_->compare(child->key(), largest->key()) > 0) {
        largest = child;
      }
    }
  }
  current_ = largest;
}

ObLsmIterator *new_merging_iterator(const ObComparator *comparator, vector<unique_ptr<ObLsmIterator>> &&children)
{
  if (children.size() == 0) {
    return nullptr;
  } else if (children.size() == 1) {
    // 只有一路时无需再包一层归并器，直接返回底层迭代器即可。
    return children[0].release();
  } else {
    return new ObMergingIterator(comparator, std::move(children));
  }
  return nullptr;
}

}  // namespace oceanbase
