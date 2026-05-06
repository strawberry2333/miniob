/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "common/lang/memory.h"
#include "oblsm/ob_user_iterator.h"
#include "oblsm/include/ob_lsm_iterator.h"
#include "oblsm/util/ob_comparator.h"
#include "oblsm/ob_lsm_define.h"
#include "oblsm/util/ob_coding.h"

namespace oceanbase {

// 这是一个“视图转换器”：
// 输入是按 internal key 排序的底层迭代器；
// 输出是按 user key 去重、按 seq 过滤、并屏蔽删除标记后的结果。
class ObUserIterator : public ObLsmIterator
{
public:
  ObUserIterator(ObLsmIterator *iter, uint64_t seq) : iter_(iter), seq_(seq), valid_(false) {}

  ~ObUserIterator() override = default;

  bool valid() const override { return valid_; }

  void seek_to_first() override
  {
    // 先把底层流定位到起点，再通过 find_next_user_entry 找到第一个可见用户记录。
    iter_->seek_to_first();
    if (iter_->valid()) {
      find_next_user_entry(false, &saved_key_);
    } else {
      valid_ = false;
    }
  }

  // 目前尚未实现反向查找，这里显式返回 invalid，避免误用。
  void seek_to_last() override { valid_ = false; }

  void seek(const string_view &target) override
  {
    // UserIterator 需要把 user key 转成 lookup key，才能在内部 key 空间中 seek。
    put_numeric<uint64_t>(&lookup_key_, target.size() + SEQ_SIZE);
    lookup_key_.append(target.data(), target.size());
    put_numeric<uint64_t>(&lookup_key_, seq_);
    iter_->seek(string_view(lookup_key_.data(), lookup_key_.size()));
    if (iter_->valid()) {
      find_next_user_entry(false, &saved_key_);
    } else {
      valid_ = false;
    }
  }

  void next() override
  {
    // 先记住当前 user key，后续扫描时要跳过它的旧版本。
    saved_key_ = extract_user_key(iter_->key());
    iter_->next();
    if (!iter_->valid()) {
      valid_ = false;
      saved_key_.clear();
      return;
    }
    find_next_user_entry(true, &saved_key_);
  }

  // 在内部有序流里找到“下一个对用户可见的 entry”。
  // skipping=true 表示当前 user key 已经被消费或删除，需要继续跳过同 key 的其余旧版本。
  void find_next_user_entry(bool skipping, std::string *skip)
  {
    do {
      size_t      curr_seq = extract_sequence(iter_->key());
      string_view user_key = extract_user_key(iter_->key());
      string_view value    = iter_->value();
      if (curr_seq <= seq_) {
        if (value.empty()) {  // tombstone：当前 user key 被删除，后续旧版本也都不可见。
          *skip    = user_key;
          skipping = true;
        } else {  // 正常 value：只有当它不属于“应跳过的旧版本”时才对外可见。
          if (skipping && user_comparator_.compare(user_key, *skip) <= 0) {
            // 仍然是同一个 user key 的旧版本，继续扫描。
          } else {
            valid_ = true;
            saved_key_.clear();
            return;
          }
        }
      }
      iter_->next();
    } while (iter_->valid());
    saved_key_.clear();
    valid_ = false;
  }

  string_view key() const override { return extract_user_key(iter_->key()); }

  string_view value() const override { return iter_->value(); }

private:
  // 底层迭代器输出的是 internal key。
  unique_ptr<ObLsmIterator> iter_;
  uint64_t                  seq_;         // 当前快照可见的最大 seq
  string                    lookup_key_;  // seek 时临时拼装的 lookup key
  string                    saved_key_;   // next/去重过程中保存的 user key
  bool                      valid_;
  ObDefaultComparator       user_comparator_;
};

ObLsmIterator *new_user_iterator(ObLsmIterator *iter, uint64_t seq) { return new ObUserIterator(iter, seq); }

}  // namespace oceanbase
