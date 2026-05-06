/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "oblsm/ob_lsm_impl.h"

#include "common/log/log.h"
#include "common/sys/rc.h"
#include "oblsm/include/ob_lsm.h"
#include "oblsm/ob_manifest.h"
#include "oblsm/ob_lsm_define.h"
#include "oblsm/wal/ob_lsm_wal.h"
#include "oblsm/table/ob_merger.h"
#include "oblsm/table/ob_sstable.h"
#include "oblsm/table/ob_sstable_builder.h"
#include "oblsm/util/ob_coding.h"
#include "oblsm/compaction/ob_compaction_picker.h"
#include "oblsm/ob_user_iterator.h"
#include "oblsm/compaction/ob_compaction.h"
#include "oblsm/ob_lsm_define.h"

namespace oceanbase {
ObLsmImpl::ObLsmImpl(const ObLsmOptions &options, const string &path)
    : options_(options), path_(path), mu_(), mem_table_(nullptr), imem_tables_(), manifest_(path)
{
  // 启动时先准备一张可写 memtable；真正的磁盘状态会在 recover() 中补齐。
  mem_table_ = make_shared<ObMemTable>();
  sstables_  = make_shared<vector<vector<shared_ptr<ObSSTable>>>>();
  if (options_.type == CompactionType::LEVELED) {
    // leveled 模式下每个下标有固定层级语义，因此提前分配层数。
    sstables_->resize(options_.default_levels);
  }

  // 后台线程目前单线程串行执行，避免 flush/compaction 并发改写全局 SSTable 视图。
  executor_.init("ObLsmBackground", 1, 1, 60 * 1000);
  block_cache_ =
      std::unique_ptr<ObLRUCache<uint64_t, shared_ptr<ObBlock>>>{new ObLRUCache<uint64_t, shared_ptr<ObBlock>>(1024)};
}

RC ObLsmImpl::recover()
{
  // 当前恢复链路分为四步：
  // 1. 打开 CURRENT 指向的 Manifest；
  // 2. 读取最后一份 snapshot 和其后的 compaction 增量；
  // 3. 按这些元数据重建 SSTable 视图与序列号；
  // 4. 若 Manifest 已积累增量历史，则立即重写为新 snapshot，缩短下一次恢复时间。
  //
  // 设计上这里还应继续结合 NewMemtable 记录重放活跃 WAL，
  // 但当前版本尚未把 recover_from_wal() 接到主链路中。
  RC rc = manifest_.open();
  if (rc != RC::SUCCESS) {
    LOG_ERROR("Failed to open manifest file, rc=%s", strrc(rc));
    return rc;
  }

  std::vector<ObManifestCompaction>      compaction_records;
  std::unique_ptr<ObManifestSnapshot>    snapshot_record;
  std::unique_ptr<ObManifestNewMemtable> new_memtable_record;

  rc = manifest_.recover(snapshot_record, new_memtable_record, compaction_records);
  if (rc != RC::SUCCESS) {
    LOG_ERROR("Failed to recover snapshot and manifest records from manifest file, rc=%s", strrc(rc));
    return rc;
  }

  // snapshot 是一次“全量磁盘视图”，恢复成本最低，因此优先加载。
  if (snapshot_record) {
    rc = load_manifest_snapshot(*snapshot_record);
    if (rc != RC::SUCCESS) {
      LOG_ERROR("Failed to load manifest snapshot, rc=%s", strrc(rc));
      return rc;
    }
  }

  // compaction record 是 snapshot 之后的增量日志，需要按顺序回放才能得到最终布局。
  rc = recover_from_manifest_records(compaction_records);
  if (rc != RC::SUCCESS) {
    LOG_ERROR("Failed to recover from manifest compaction records, rc=%s", strrc(rc));
    return rc;
  }

  // 这里理论上应当基于 new_memtable_record 打开当前活跃 WAL，
  // 并把“已写 WAL、未刷 SSTable”的那段数据重建回 memtable。
  // 当前实现仅先创建活跃 WAL 对象，后续逻辑仍待补齐。
  wal_ = std::make_unique<WAL>();

  // 若本次是靠“snapshot + 若干增量”恢复出来的，就立即把结果压缩回一份新 snapshot。
  if (!compaction_records.empty()) {
    rc = write_manifest_snapshot();
    if (rc != RC::SUCCESS) {
      LOG_ERROR("Failed to write snapshot into manifest file, rc=%s", strrc(rc));
      return rc;
    }
  }

  return RC::SUCCESS;
}

RC ObLsm::open(const ObLsmOptions &options, const string &path, ObLsm **dbptr)
{
  RC         rc  = RC::SUCCESS;
  ObLsmImpl *lsm = new ObLsmImpl(options, path);
  *dbptr         = lsm;
  rc             = lsm->recover();
  return rc;
}

RC ObLsmImpl::put(const string_view &key, const string_view &value)
{
  // 当前写入路径比较直接：
  // 1. 在锁内分配全局 seq；
  // 2. 先写 WAL，必要时立刻 sync；
  // 3. 再把记录写入 memtable；
  // 4. 若 memtable 超过阈值，则尝试 freeze 并交给后台刷盘。
  //
  // 这里还没有做真正的写限速；当 immutable memtable 尚未刷完时，
  // 前台线程会直接阻塞等待，而不是按平滑节流方式退让。
  LOG_TRACE("begin to put key=%s, value=%s", key.data(), value.data());
  RC rc = RC::SUCCESS;
  // 当前 memtable 底层是 skiplist，实现未提供并发写安全，因此整个 put 过程受同一把锁保护。
  unique_lock<mutex> lock(mu_);
  // seq_ 是全局单调递增版本号；fetch_add 返回的是“本次写入实际使用的旧值”。
  uint64_t           seq = seq_.fetch_add(1);
  // 先落 WAL，再更新内存，这是崩溃恢复语义的基本前提。
  rc = wal_->put(seq, key, value);
  if (rc != RC::SUCCESS) {
    return rc;
  }

  if (options_.force_sync_new_log) {
    // 强同步模式下，每条日志都尽量在返回前持久化。
    rc = wal_->sync();
    if (rc != RC::SUCCESS) {
      LOG_ERROR("Failed to sync wal logs, rc=%s", strrc(rc));
      return rc;
    }
  }
  // WAL 成功后再更新 memtable，确保内存态始终能被日志重建。
  mem_table_->put(seq, key, value);
  size_t mem_size = mem_table_->appro_memory_usage();
  if (mem_size > options_.memtable_size) {
    // 当前实现一次最多只允许一个 immutable memtable 等待后台刷盘。
    // 如果前一轮 flush 还没完成，新的写线程会在这里等待。
    if (imem_tables_.size() >= 1) {
      cv_.wait(lock);
    }
    // 醒来后必须重新检查：可能别的写线程已经完成了 freeze。
    if (mem_table_->appro_memory_usage() > options_.memtable_size) {
      // Manifest 里的 compaction record 会引用 latest_seq，因此在切换前先对齐。
      manifest_.latest_seq = seq;
      try_freeze_memtable();
    } else {
      // 若别的线程已经腾出了新 memtable，这里继续唤醒一个等待写线程。
      cv_.notify_one();
    }
  }
  return rc;
}

RC ObLsmImpl::batch_put(const vector<pair<string, string>> &kvs) { return RC::UNIMPLEMENTED; }

RC ObLsmImpl::remove(const string_view &key) { return RC::UNIMPLEMENTED; }

RC ObLsmImpl::try_freeze_memtable()
{
  RC rc = RC::SUCCESS;
  // 先把当前可写 memtable 变成 immutable，后续后台线程会消费它。
  imem_tables_.emplace_back(mem_table_);
  mem_table_ = make_unique<ObMemTable>();
  // 非强同步模式下，freeze 是旧 WAL 变成“可刷盘输入”前的最后一次持久化边界。
  if (!options_.force_sync_new_log) {
    rc = wal_->sync();
    if (rc != RC::SUCCESS) {
      LOG_ERROR("Failed to sync wal logs, rc=%s", strrc(rc));
      return rc;
    }
  }

  // 旧 WAL 必须和旧 memtable 成对保存，直到后台 flush 成功后才能一起删除。
  frozen_wals_.emplace_back(std::move(wal_));
  wal_                     = std::make_unique<WAL>();
  // 新 memtable 从新的 WAL 文件继续接写；Manifest 后续会记录这个“新的活跃编号”。
  uint64_t new_memtable_id = memtable_id_.fetch_add(1) + 1;
  wal_->open(get_wal_path(new_memtable_id));
  std::shared_ptr<ObLsmBgCompactCtx> background_compaction_ctx = make_shared<ObLsmBgCompactCtx>(new_memtable_id);
  auto bg_task = [this, background_compaction_ctx]() { this->background_compaction(background_compaction_ctx); };
  int  ret     = executor_.execute(bg_task);
  if (ret != 0) {
    // 这里没有回滚已完成的内存/日志切换，因此失败意味着系统进入了“有 immutable 待刷盘但后台任务未提交”的异常状态。
    rc = RC::INTERNAL;
    LOG_WARN("fail to execute background compaction task");
  }
  return rc;
}

void ObLsmImpl::background_compaction(std::shared_ptr<ObLsmBgCompactCtx> ctx)
{
  unique_lock<mutex> lock(mu_);
  if (imem_tables_.size() >= 1) {
    shared_ptr<ObMemTable> imem       = imem_tables_.back();
    shared_ptr<WAL>        frozen_wal = frozen_wals_.back();

    // 当前实现把“取出 imem/wal、构建 SST、更新元数据”整体放在同一临界区中，
    // 逻辑最直接，但会拉长前台写线程等待时间。后续可再细化锁粒度。
    build_sstable(imem);
    imem_tables_.pop_back();
    frozen_wals_.pop_back();
    // flush 成功后，Manifest 里的活跃 memtable 指针可以前移到新的 WAL 编号。
    manifest_.push(ObManifestNewMemtable{ctx->new_memtable_id});

    // 旧 WAL 所承载的数据已经进入 SSTable，可安全删除。
    ::remove(frozen_wal->filename().c_str());

    lock.unlock();
    // 释放等待写线程：现在系统重新回到“只有一个活跃 memtable”的稳定状态。
    cv_.notify_one();

    // flush 之后立刻检查是否需要继续做 major compaction。
    // 当前触发点较少，只在后台 flush 完成后尝试一次。
    if (!compacting_) {
      compacting_.store(true);
      try_major_compaction();
      compacting_.store(false);
    }
    return;
  }
}

void ObLsmImpl::try_major_compaction()
{
  unique_lock<mutex>             lock(mu_);
  // picker 只读取当前视图并挑选任务，不执行真正的文件合并。
  unique_ptr<ObCompactionPicker> picker(ObCompactionPicker::create(options_.type, &options_));
  unique_ptr<ObCompaction>       picked = picker->pick(sstables_);
  ObManifestCompaction           mf_record;
  lock.unlock();
  if (picked == nullptr || picked->size() == 0) {
    return;
  }
  // 真正的 merge/重写文件在锁外完成，避免长时间阻塞 put/get。
  vector<shared_ptr<ObSSTable>> results = do_compaction(picked.get());

  SSTablesPtr new_sstables = make_shared<vector<vector<shared_ptr<ObSSTable>>>>();
  lock.lock();
  size_t levels_size        = sstables_->size();
  bool   insert_new_sstable = false;
  auto   find_sstable       = [](const vector<shared_ptr<ObSSTable>> &picked, const shared_ptr<ObSSTable> &sstable) {
    for (auto &p : picked) {
      if (p->sst_id() == sstable->sst_id()) {
        return true;
      }
    }
    return false;
  };

  vector<shared_ptr<ObSSTable>> picked_sstables;
  picked_sstables      = picked->inputs(0);
  const auto &level_i1 = picked->inputs(1);
  if (level_i1.size() > 0) {
    picked_sstables.insert(picked_sstables.end(), level_i1.begin(), level_i1.end());
  }
  // 把“旧布局减去 picked 输入，再插入 compaction 结果”重建成新的内存视图。
  // 不同策略对层级语义不同，因此当前分别处理。
  if (options_.type == CompactionType::TIRED) {
    for (int i = levels_size - 1; i >= 0; --i) {
      const vector<shared_ptr<ObSSTable>> &level_i = sstables_->at(i);
      for (auto &sstable : level_i) {
        if (find_sstable(picked_sstables, sstable)) {
          if (!insert_new_sstable) {
            new_sstables->insert(new_sstables->begin(), results);
            insert_new_sstable = true;
          }
        } else {
          new_sstables->insert(new_sstables->begin(), level_i);
          break;
        }
      }
    }
  } else if (options_.type == CompactionType::LEVELED) {
    // TODO: 这里还需要把 leveled compaction 的输出按目标层级写回新的 SSTable 视图。
  }

  // 到这里为止，内存中的 SSTable 视图已经完成原子切换。
  sstables_ = new_sstables;
  lock.unlock();

  // 再删除磁盘上的旧输入文件，避免恢复时重新看到这些已被替换的表。
  for (auto &sstable : picked_sstables) {
    sstable->remove();
  }

  // 把本次 compaction 的结果追加进 Manifest，供后续恢复回放。
  mf_record.sstable_sequence_id = sstable_id_.load();
  mf_record.seq_id              = manifest_.latest_seq;
  manifest_.push(std::move(mf_record));
  // 继续尝试下一轮，直到 picker 判断当前布局已经稳定。
  try_major_compaction();
}

// 当前 compaction 合并逻辑尚未实现；这里保留占位函数，方便上层调度链路先打通。
vector<shared_ptr<ObSSTable>> ObLsmImpl::do_compaction(ObCompaction *picked) { return {}; }

void ObLsmImpl::build_sstable(shared_ptr<ObMemTable> imem)
{
  // flush 的输入来自已经冻结的 immutable memtable，因此这里无需考虑并发写入。
  unique_ptr<ObSSTableBuilder> tb = make_unique<ObSSTableBuilder>(&default_comparator_, block_cache_.get());

  uint64_t sstable_id = sstable_id_.fetch_add(1);
  tb->build(imem, get_sstable_path(sstable_id), sstable_id);

  // 这条 Manifest compaction record 负责把“新 SSTable 已经加入布局”持久化下来。
  ObManifestCompaction record;
  record.compaction_type     = options_.type;
  record.sstable_sequence_id = sstable_id_.load();
  record.seq_id              = manifest_.latest_seq;

  // flush 完成后的 SSTable 摆放位置依赖 compaction 策略：
  // - tiered: 作为新的 run 插到最前面；
  // - leveled: 先进入 L0，并记录一条 added_tables 元数据。
  if (options_.type == CompactionType::TIRED) {
    // TODO: tiered 模式尚未把 flush 结果写成 Manifest 增量，因此恢复链路暂时不完整。
    // 这里的“level”更接近 run 的概念。
    sstables_->insert(sstables_->begin(), {tb->get_built_table()});
  } else if (options_.type == CompactionType::LEVELED) {
    sstables_->at(0).emplace_back(tb->get_built_table());
    record.added_tables.emplace_back(sstable_id, 0);
    manifest_.push(std::move(record));
  }
}

string ObLsmImpl::get_sstable_path(uint64_t sstable_id)
{
  return filesystem::path(path_) / (to_string(sstable_id) + SSTABLE_SUFFIX);
}

string ObLsmImpl::get_wal_path(uint64_t memtable_id)
{
  return filesystem::path(path_) / (to_string(memtable_id) + WAL_SUFFIX);
}

RC ObLsmImpl::get(const string_view &key, string *value)
{
  RC                 rc = RC::SUCCESS;
  unique_lock<mutex> lock(mu_);
  // 读路径复用统一迭代器：先把所有来源归并，再按 user key/seq 规则过滤。
  auto               iter = unique_ptr<ObLsmIterator>(new_iterator(ObLsmReadOptions{}));
  iter->seek(key);
  if (iter->valid() && iter->key() == key) {
    if (iter->value().empty()) {
      rc = RC::NOT_EXIST;
    } else {
      value->assign(iter->value());
    }
  } else {
    rc = RC::NOT_EXIST;
  }
  return rc;
}

ObLsmIterator *ObLsmImpl::new_iterator(ObLsmReadOptions options)
{
  unique_lock<mutex>     lock(mu_);
  // 先在锁内抓取一个稳定视图，然后尽快释放锁，避免迭代器构造和后续遍历长时间阻塞写线程。
  shared_ptr<ObMemTable> mem = mem_table_;

  shared_ptr<ObMemTable> imm = nullptr;
  if (!imem_tables_.empty()) {
    imm = imem_tables_.back();
  }
  vector<shared_ptr<ObSSTable>> sstables;
  for (auto &level : *sstables_) {
    sstables.insert(sstables.end(), level.begin(), level.end());
  }
  lock.unlock();
  vector<unique_ptr<ObLsmIterator>> iters;
  // 构造顺序体现了“新数据优先覆盖旧数据”的意图：memtable -> immutable -> SSTables。
  iters.emplace_back(mem->new_iterator());
  if (imm != nullptr) {
    iters.emplace_back(imm->new_iterator());
  }
  for (const auto &sst : sstables) {
    iters.emplace_back(sst->new_iterator());
  }

  return new_user_iterator(
      new_merging_iterator(&internal_key_comparator_, std::move(iters)), options.seq == -1 ? seq_.load() : options.seq);
}

ObLsmTransaction *ObLsmImpl::begin_transaction() { return new ObLsmTransaction(this, seq_.load()); }

void ObLsmImpl::dump_sstables()
{
  unique_lock<mutex> lock(mu_);
  int                level = sstables_->size();
  for (int i = 0; i < level; i++) {
    cout << "level " << i << endl;
    int level_size = 0;
    for (auto &sst : sstables_->at(i)) {
      cout << sst->sst_id() << ": " << sst->size() << ";";
      level_size += sst->size();
    }
    cout << "level size " << level_size << endl;
  }
}

RC ObLsmImpl::recover_from_manifest_records(const std::vector<ObManifestCompaction> &records)
{
  // 先在纯编号层面回放增量记录，得到“每个 level 最终有哪些 SSTable”。
  // 等布局算完后，再统一打开磁盘文件，避免恢复过程中频繁创建/销毁 SSTable 对象。
  std::vector<std::vector<uint64_t>> tmp_sstables;
  for (size_t i = 0; i < options_.default_levels; i++) {
    tmp_sstables.emplace_back();
  }
  for (auto &record : records) {
    // 恢复全局编号分配器时，以记录里携带的上界为准。
    sstable_id_ = record.sstable_sequence_id;
    seq_        = record.seq_id;
    // 先应用新增表。
    for (auto &info : record.added_tables) {
      uint32_t level      = info.level;
      uint64_t sstable_id = info.sstable_id;
      ASSERT(level < options_.default_levels, "level shouldn't greater than or equal to default level size");
      tmp_sstables[level].push_back(sstable_id);
    }
    // 再删除已被 compaction 吃掉的旧表。
    for (auto &info : record.deleted_tables) {
      uint32_t level = info.level;
      uint32_t sid   = info.sstable_id;
      ASSERT(level < options_.default_levels, "level shouldn't greater than or equal to default level size");
      auto del_iter = std::find(tmp_sstables[level].begin(), tmp_sstables[level].end(), sid);
      tmp_sstables[level].erase(del_iter);
    }
  }

  RC rc = load_manifest_sstable(tmp_sstables);
  if (rc != RC::SUCCESS) {
    LOG_ERROR("Failed to load sstables, rc=%s", strrc(rc));
    return rc;
  }
  return RC::SUCCESS;
}

RC ObLsmImpl::load_manifest_snapshot(const ObManifestSnapshot &snapshot)
{
  // 快照直接给出“某一时刻的全量状态”，因此恢复时可以一次性重设全局编号。
  seq_        = snapshot.seq;
  sstable_id_ = snapshot.sstable_id;
  RC rc       = load_manifest_sstable(snapshot.sstables);
  return rc;
}

RC ObLsmImpl::load_manifest_sstable(const std::vector<std::vector<uint64_t>> &sstables)
{
  // 根据 Manifest 中的编号重新打开 SSTable 文件，把纯元数据视图恢复成可读对象。
  // 这一步不重建文件内容，只重建内存句柄；真正的数据仍然来自磁盘上的现有 `.sst` 文件。
  size_t cur_level_idx = 0;
  for (auto &sst_ids : sstables) {
    auto &cur_level = sstables_->at(cur_level_idx++);
    for (auto &sst_id : sst_ids) {
      auto filename = get_sstable_path(sst_id);
      auto sstable  = std::make_shared<ObSSTable>(sst_id, filename, &default_comparator_, block_cache_.get());
      sstable->init();
      cur_level.emplace_back(sstable);
    }
  }
  return RC::SUCCESS;
}

RC ObLsmImpl::write_manifest_snapshot()
{
  ObManifestSnapshot    snapshot;
  ObManifestNewMemtable new_memtable;
  // snapshot 记录的是“此刻完整磁盘视图 + 全局编号”，供后续恢复直接使用。
  snapshot.seq             = seq_.load();
  snapshot.sstable_id      = sstable_id_.load();
  snapshot.compaction_type = options_.type;
  snapshot.sstables.resize(sstables_->size());
  // new_memtable 记录的是“压缩历史之后当前仍然活跃的 WAL 编号”。
  new_memtable.memtable_id = memtable_id_.load();
  for (size_t i = 0; i < sstables_->size(); ++i) {
    auto &level = sstables_->at(i);
    for (size_t j = 0; j < level.size(); ++j) {
      snapshot.sstables[i].push_back(level[j]->sst_id());
    }
  }
  RC rc = manifest_.redirect(snapshot, new_memtable);
  if (OB_FAIL(rc)) {
    LOG_ERROR("Failed to redirect snapshot");
    return rc;
  }
  return RC::SUCCESS;
}

}  // namespace oceanbase
