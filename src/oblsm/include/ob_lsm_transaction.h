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
#include "common/lang/string_view.h"
#include "common/lang/string.h"
#include "oblsm/include/ob_lsm_iterator.h"
#include "oblsm/include/ob_lsm_options.h"
#include "oblsm/include/ob_lsm.h"
#include "oblsm/memtable/ob_skiplist.h"
#include "oblsm/util/ob_arena.h"
#include "oblsm/util/ob_coding.h"
#include "common/lang/map.h"

namespace oceanbase {

/**
 * @class ObLsmTransaction
 * @brief oblsm 的事务抽象。
 *
 * 当前事务能力还没有完全落地，这个类更多承担两层职责：
 * - 对外固定事务 API 形状，让调用方知道未来会提供哪些能力；
 * - 对内明确事务实现需要维护的关键状态，例如快照时间戳和未提交写集合。
 *
 * 按设计意图，事务应当基于 `ts_` 提供快照读，并把未提交改动先缓存在
 * `inner_store_` 中，提交时再一次性写入底层 LSM。当前 `.cpp` 中大部分方法
 * 仍返回 `RC::UNIMPLEMENTED`，因此这里的注释重点是解释职责和后续实现边界。
 */
class ObLsmTransaction
{
public:
  /**
   * @brief 构造事务对象。
   *
   * 构造时把事务绑定到某个数据库实例，并记录事务开始时看到的序列号/时间戳。
   * 后续无论是点查还是范围扫描，都应当以这个时间戳作为默认可见性上界。
   *
   * @param db 事务作用的底层数据库。
   * @param ts 事务开始时捕获的快照时间戳。
   */
  ObLsmTransaction(ObLsm *db, uint64_t ts);

  ~ObLsmTransaction() = default;

  /**
   * @brief 在事务视图中读取指定 key。
   *
   * 正确实现时应当遵循“先事务内、后底层快照”的查找顺序：
   * 1. 先检查 `inner_store_` 是否存在未提交的 put/remove；
   * 2. 未命中时，再以 `ts_` 为快照上界访问底层 LSM；
   * 3. 将两部分结果合成为最终对事务可见的值。
   *
   * @param key 待查找的用户键。
   * @param value 命中时写回对应值。
   */
  RC get(const string_view &key, string *value);

  /**
   * @brief 在事务私有缓冲区中新增或更新一条记录。
   *
   * 事务内写入不应直接修改底层 LSM；只有这样，`rollback()` 才能简单丢弃，
   * `commit()` 才能把整批改动视为一个逻辑单元提交。
   */
  RC put(const string_view &key, const string_view &value);

  /**
   * @brief 在事务范围内删除一个 key。
   *
   * 删除通常会先写入事务内删除标记，而不是立即触发底层 remove。
   * 这样点查、迭代和提交都能基于统一的事务写集合来解释“这个 key 是否仍可见”。
   */
  RC remove(const string_view &key);

  /**
   * @brief 创建事务视图迭代器。
   *
   * 理想行为是把“底层数据库快照迭代器”和“事务内 map 迭代器”做一次归并：
   * - 同 key 时优先返回事务内版本；
   * - 删除标记应屏蔽底层旧值；
   * - 快照上界应当同时受 `options.seq` 与事务 `ts_` 约束。
   *
   * @param options 读选项。
   * @return 新建迭代器对象，生命周期由调用方管理。
   */
  ObLsmIterator *new_iterator(ObLsmReadOptions options);

  /**
   * @brief 提交事务，把所有未提交改动持久化到底层 LSM。
   *
   * 易错点在于提交顺序必须稳定，否则同事务内多次更新同 key 时，
   * 最终写入的版本顺序可能与事务内部看到的不一致。
   */
  RC commit();

  /**
   * @brief 回滚事务，丢弃全部未提交改动。
   */
  RC rollback();

private:
  // 事务关联的底层数据库。事务最终仍要把改动落到这个实例上。
  ObLsm *db_ = nullptr;

  // 事务开始时捕获的快照时间戳，决定默认读取上界。
  uint64_t ts_ = 0;

  // 事务内暂存的未提交写集合。
  // 易错点：这里既可能保存正常值，也可能借助约定表达“删除”，所以不能简单把空字符串等同于不存在。
  map<string, string> inner_store_;
};

/**
 * @class TrxInnerMapIterator
 * @brief 事务内部 map 迭代器的占位类型。
 *
 * 当前类体为空，说明事务范围扫描尚未实现；保留该类型是为了固定“事务内写集合也要适配统一迭代器接口”
 * 这一设计方向。
 */
class TrxInnerMapIterator : public ObLsmIterator
{};
}  // namespace oceanbase
