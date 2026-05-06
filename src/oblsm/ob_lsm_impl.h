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

#include "oblsm/include/ob_lsm.h"

#include "common/lang/mutex.h"
#include "common/lang/atomic.h"
#include "common/lang/memory.h"
#include "common/lang/condition_variable.h"
#include "common/lang/utility.h"
#include "common/thread/thread_pool_executor.h"
#include "oblsm/include/ob_lsm_transaction.h"
#include "oblsm/memtable/ob_memtable.h"
#include "oblsm/table/ob_sstable.h"
#include "oblsm/util/ob_lru_cache.h"
#include "oblsm/compaction/ob_compaction.h"
#include "oblsm/ob_manifest.h"
#include "oblsm/wal/ob_lsm_wal.h"

namespace oceanbase {

/**
 * @brief 后台 flush/compaction 任务携带的上下文。
 *
 * 当前后台线程真正需要的业务参数不多，核心就是“切换后的新 memtable_id”。
 * 之所以单独包装成对象，而不是直接把整数塞进 lambda，是为了让这份上下文在
 * 前台线程解锁、继续接受新写入之后仍然稳定可用。
 */
struct ObLsmBgCompactCtx
{
  ObLsmBgCompactCtx() = default;
  ObLsmBgCompactCtx(uint64_t id) : new_memtable_id(id) {}
  // 这不是被冻结的旧 memtable 编号，而是 freeze 完成后“新的活跃 memtable/WAL 编号”。
  // 后台任务在刷盘结束后，会把这个编号写入 Manifest，告诉恢复流程下次应从哪个 WAL 继续接写。
  uint64_t new_memtable_id;
};

/**
 * @brief ObLsm 的核心实现，负责调度写入、后台刷盘、compaction 与恢复。
 *
 * 这个类维持了 oblsm 当前最关键的几条状态链路：
 * - 写入链路：`WAL -> MemTable`
 * - flush 链路：`MemTable(full) -> Immutable MemTable -> SSTable`
 * - 元数据链路：`Manifest snapshot/compaction/new_memtable`
 * - 读链路：`MemTable/Immutable/SSTable 多路归并 -> UserIterator`
 *
 * 设计上它更像一个“小型调度器”，而不只是简单的 KV 容器：
 * 前台线程负责追加写和触发 freeze，后台线程负责把冻结的 memtable 落成 SSTable，
 * 恢复逻辑则负责在重启时把磁盘视图、序列号和活跃 WAL 重新对齐。
 */
class ObLsmImpl : public ObLsm
{
public:
  ObLsmImpl(const ObLsmOptions &options, const string &path);
  /**
   * @brief 关闭实例时收尾后台工作并补做一次 WAL sync。
   *
   * 当前实现没有显式的 `close()`，因此析构函数承担“尽量把异步任务停干净”的职责。
   * 当 `force_sync_new_log=false` 时，这里还会主动对当前 WAL 做一次同步，减少最近日志丢失窗口。
   */
  ~ObLsmImpl() override
  {
    if (!options_.force_sync_new_log) {
      wal_->sync();
    }
    executor_.shutdown();
    executor_.await_termination();
  }

  RC put(const string_view &key, const string_view &value) override;

  RC get(const string_view &key, string *value) override;

  RC remove(const string_view &key) override;

  ObLsmTransaction *begin_transaction() override;

  ObLsmIterator *new_iterator(ObLsmReadOptions options) override;

  SSTablesPtr get_sstables() { return sstables_; }

  /**
   * @brief 恢复 LSM 的内存态与磁盘态。
   *
   * 当前实现优先恢复 Manifest 里描述的 SSTable 视图和序列号，
   * WAL 重放链路已预留接口，但本实现中尚未真正接入。
   */
  RC recover();
  RC batch_put(const std::vector<pair<string, string>> &kvs) override;

  // 调试辅助：输出当前内存中的层级/运行布局。
  void dump_sstables() override;

private:
  // 预留给“按 Manifest 记录定位活跃 WAL 并重放”的恢复步骤。
  // 当前版本尚未提供实现，因此 recover() 只恢复 Manifest 部分状态。
  RC recover_from_wal();
  // 回放 snapshot 之后残留的 compaction 增量记录，补齐最终 SSTable 视图。
  RC recover_from_manifest_records(const std::vector<ObManifestCompaction> &records);
  // 直接按快照覆盖内存中的 SSTable 布局和关键序列号。
  RC load_manifest_snapshot(const ObManifestSnapshot &snapshot);
  // 根据 Manifest 中记录的 SSTable 编号重新打开磁盘文件并构造内存句柄。
  RC load_manifest_sstable(const std::vector<std::vector<uint64_t>> &sstables);
  // 用“最新全量快照 + 当前活跃 memtable 指针”重写 Manifest，缩短恢复链路。
  RC write_manifest_snapshot();

private:
  /**
   * @brief 尝试冻结当前活跃 MemTable，并切换到新的 WAL/MemTable。
   *
   * 这是前台写线程和后台刷盘线程的分界点：
   * - 前台线程把当前 `mem_table_` 放入 `imem_tables_`；
   * - 旧 WAL 也同步转入 `frozen_wals_`；
   * - 然后创建一套新的 `(mem_table_, wal_)` 继续接收写入；
   * - 最后投递后台任务，把冻结出来的那一批数据刷成 SSTable。
   *
   * 当前实现的关键边界条件是：同一时刻最多只允许一个 immutable memtable 等待刷盘，
   * 因此前台线程在 `put()` 中可能会因为 `imem_tables_.size() >= 1` 而阻塞。
   */
  RC try_freeze_memtable();

  /**
   * @brief 执行一次 compaction 计划，并返回产出的新 SSTable 集合。
   *
   * 输入 `compaction` 只描述“要合哪些表”，真正的 merge/重写文件在这里完成。
   * 返回值不直接落入 `sstables_`，而是先交给调用方在持锁状态下把新旧布局原子替换，
   * 避免外部在更新全局视图前读到半成品结果。
   */
  vector<shared_ptr<ObSSTable>> do_compaction(ObCompaction *compaction);

  /**
   * @brief 尝试挑选并执行一轮 major compaction。
   *
   * 该流程大体分成三段：
   * 1. 在锁内基于当前 `sstables_` 选择一项 compaction；
   * 2. 在锁外真正执行 merge，避免长时间阻塞写入；
   * 3. 回到锁内替换 SSTable 视图，并把结果写入 Manifest。
   *
   * 当前实现会在成功完成一轮后递归调用自身，直到 picker 再也挑不出任务。
   * 易错点在于：新旧文件替换、Manifest 记录和磁盘删除三者的顺序必须保持一致。
   */
  void try_major_compaction();

  /**
   * @brief 后台线程入口：把冻结的 immutable memtable 落成 SSTable，并在必要时继续 compaction。
   *
   * 这里处理的是“flush + 后续调度”而不只是 compaction：
   * 先把 imem 刷盘、更新 Manifest 和 WAL 状态，再视情况触发 major compaction。
   */
  void background_compaction(std::shared_ptr<ObLsmBgCompactCtx> ctx);

  /**
   * @brief 把一个 immutable memtable 构建成 SSTable。
   *
   * 这是 flush 链路的核心动作。除写出 `.sst` 文件外，它还会按 compaction 策略
   * 维护 `sstables_` 的内存布局，并把新表信息写入 Manifest 增量记录。
   */
  void build_sstable(shared_ptr<ObMemTable> imem);

  /**
   * @brief 根据 SSTable 编号拼出磁盘文件路径。
   */
  string get_sstable_path(uint64_t sstable_id);

  /**
   * @brief 根据 memtable 编号拼出对应 WAL 文件路径。
   *
   * 这里的 memtable 编号同时也承担“活跃 WAL 编号”的职责，
   * Manifest 里的 `ObManifestNewMemtable` 就是依靠这套命名规则定位日志文件。
   */
  string get_wal_path(uint64_t memtable_id);

  ObLsmOptions                      options_;
  // 当前数据库实例的数据目录，Manifest/SST/WAL 都以它为根路径。
  string                            path_;
  // 保护 memtable、immutable 队列、sstable 视图等共享状态。
  mutex                             mu_;
  // 当前活跃 WAL，对应仍在接受写入的 memtable。
  std::shared_ptr<WAL>              wal_;
  // 已冻结但尚未被后台线程回收删除的 WAL。
  std::vector<std::shared_ptr<WAL>> frozen_wals_;
  // 当前可写 memtable。
  shared_ptr<ObMemTable>            mem_table_;
  // 已冻结、等待刷盘的 immutable memtable 队列。当前实现通常只允许队列长度为 0 或 1。
  vector<shared_ptr<ObMemTable>>    imem_tables_;
  // 当前内存中的 SSTable 视图；不同 compaction 模式下层级含义可能不同。
  SSTablesPtr                       sstables_;
  // 单线程后台执行器，串行处理 flush/compaction，避免并发修改全局 SSTable 视图。
  common::ThreadPoolExecutor        executor_;
  // 元数据日志管理器，负责记录 snapshot、compaction 结果和当前活跃 memtable/WAL。
  ObManifest                        manifest_;
  // 下一个写入将分配到的全局序列号。
  atomic<uint64_t>                  seq_{0};
  // 下一个新建 SSTable 将使用的编号上界。
  atomic<uint64_t>                  sstable_id_{0};
  // 当前活跃 memtable/WAL 的编号。
  atomic<uint64_t>                  memtable_id_{0};
  // 当前台线程因 immutable memtable 尚未刷盘而等待时，用它来唤醒继续写入。
  condition_variable                cv_;
  // TODO: use global variable?
  // 用户 key 比较器，供构建 SSTable 或用户态逻辑使用。
  const ObDefaultComparator                                  default_comparator_;
  // internal key 比较器，负责把 `(user_key, seq)` 排成符合版本语义的顺序。
  const ObInternalKeyComparator                              internal_key_comparator_;
  // 防止多个后台路径重复触发 major compaction。
  atomic<bool>                                               compacting_ = false;
  // Block 级缓存，供 SSTable 读路径复用。
  std::unique_ptr<ObLRUCache<uint64_t, shared_ptr<ObBlock>>> block_cache_;
};

}  // namespace oceanbase
