/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "oblsm/include/ob_lsm_transaction.h"
#include "oblsm/util/ob_comparator.h"
#include "common/lang/memory.h"

namespace oceanbase {

/**
 * @brief 事务迭代器合并器的占位实现。
 *
 * 预期用途是把两路有序流合成一条事务可见流：
 * - `left_` 通常代表事务内未提交改动；
 * - `right_` 通常代表底层数据库的快照读结果。
 *
 * 合并时如果两边出现相同 key，应优先返回 `left_`，因为事务内新值或删除标记
 * 需要覆盖底层旧版本。当前类方法尚未实现，这里的注释主要用来说明后续设计意图。
 */
class TrxIterator : public ObLsmIterator
{
public:
  TrxIterator(ObLsmIterator *left, ObLsmIterator *right) : left_(left), right_(right) {}
  ~TrxIterator() override = default;

  bool valid() const override { return false; }
  void seek_to_first() override {}
  void seek_to_last() override {}
  void seek(const string_view &key) override {}
  void next() override {}

  string_view key() const override { return ""; }
  string_view value() const override { return ""; }

private:
  // 左侧一般放事务内数据源，优先级更高。
  unique_ptr<ObLsmIterator> left_;
  // 右侧一般放底层数据库快照迭代器。
  unique_ptr<ObLsmIterator> right_;
};

ObLsmTransaction::ObLsmTransaction(ObLsm *db, uint64_t ts) : db_(db), ts_(ts)
{
  // 当前构造函数只保存上下文；后续若引入事务状态机，可以从这里初始化。
  (void)db_;
  (void)ts_;
}

RC ObLsmTransaction::get(const string_view &key, string *value) { return RC::UNIMPLEMENTED; }

RC ObLsmTransaction::put(const string_view &key, const string_view &value) { return RC::UNIMPLEMENTED; }

RC ObLsmTransaction::remove(const string_view &key) { return RC::UNIMPLEMENTED; }

ObLsmIterator *ObLsmTransaction::new_iterator(ObLsmReadOptions options)
{
  // 正式实现时需要让 `options.seq` 与 `ts_` 协同工作，避免事务越过自己的快照边界。
  return nullptr;
}

RC ObLsmTransaction::commit() { return RC::UNIMPLEMENTED; }

RC ObLsmTransaction::rollback() { return RC::UNIMPLEMENTED; }

}  // namespace oceanbase
