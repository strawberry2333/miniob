/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
   miniob is licensed under Mulan PSL v2.
   You can use this software according to the terms and conditions of the Mulan PSL v2.
   You may obtain a copy of Mulan PSL v2 at:
            http://license.coscl.org.cn/MulanPSL2
   THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
   EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
   MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
   See the Mulan PSL v2 for more details. */

// WAL 接口定义。
// 这里把“日志记录长什么样”和“日志文件对外提供哪些操作”集中在一起，
// 便于上层写路径和恢复路径共享同一套语义。
#pragma once

#include "common/lang/mutex.h"
#include "common/sys/rc.h"
#include "oblsm/util/ob_file_writer.h"

namespace oceanbase {

/**
 * @struct WalRecord
 * @brief 预写日志（WAL）中的一条记录，包含序列号、键和值。
 */
struct WalRecord
{
  /** 记录的序列号，用于标识日志的写入顺序，也是恢复时的重放顺序。 */
  uint64_t seq;
  /** 记录对应的键。 */
  std::string key;
  /** 记录对应的值。若上层将来支持 tombstone，这里也可能承载删除标记。 */
  std::string val;

  /**
   * @brief 构造一条 WAL 记录。
   *
   * @param s 序列号
   * @param k 键
   * @param v 值
   */
  WalRecord(uint64_t s, std::string k, std::string v) : seq(s), key(std::move(k)), val(std::move(v)) {}
};

/**
 * @class WAL
 * @brief 负责预写日志（WAL）的写入与恢复。
 *
 * WAL 保证数据写入主存储前先持久化到日志文件，系统崩溃后可通过重放日志恢复 memtable。
 *
 * ### 序列化格式：
 * 每条记录按以下顺序写入文件：
 * - **序列号（uint64_t）**：8 字节，标识日志顺序。
 * - **Key 长度（size_t）**：键的字节长度。
 * - **Key（string）**：键的内容。
 * - **Value 长度（size_t）**：值的字节长度。
 * - **Value（string）**：值的内容。
 *
 * 在理想实现里，写路径应先追加 WAL，再写入 MemTable。
 * 是否每次都强制 `sync`，由 `ObLsmOptions::force_sync_new_log` 控制。
 */
class WAL
{
public:
  WAL() {}

  ~WAL() = default;

  /**
   * @brief 打开 WAL 文件，准备追加写入。
   *
   * 通常一个活跃 memtable 对应一个 WAL 文件；memtable freeze 之后，
   * 新 memtable 会切到新的 WAL 文件继续写。
   *
   * 这个接口关注的是“日志生命周期切换”，不是恢复逻辑本身。
   * 恢复场景通常直接调用 `recover()` 扫描已有文件。
   *
   * @param filename WAL 文件路径
   * @return 成功返回 `RC::SUCCESS`，否则返回错误码。
   */
  RC open(const std::string &filename) { return RC::UNIMPLEMENTED; }

  /**
   * @brief 从指定 WAL 文件中恢复记录。
   *
   * 读取文件中的所有键值对，追加到 `wal_records` 中。
   * 调用方通常会按返回顺序把记录重新灌入 memtable，从而找回
   * “已经写入 WAL 但还未来得及刷成 SSTable”的那部分数据。
   *
   * @param wal_file WAL 文件路径
   * @param wal_records 用于存放恢复出的记录
   * @return 成功返回 `RC::SUCCESS`，否则返回错误码。
   */
  RC recover(const std::string &wal_file, std::vector<WalRecord> &wal_records);

  /**
   * @brief 向 WAL 写入一条键值记录。
   *
   * 这一步的职责只有“先持久化 redo 信息”。
   * 真正把键值放入 memtable、触发 freeze 或 flush，属于更上层的职责。
   *
   * @param seq 序列号
   * @param key 键
   * @param val 值
   * @return 成功返回 `RC::SUCCESS`，否则返回错误码。
   */
  RC put(uint64_t seq, std::string_view key, std::string_view val);

  /**
   * @brief 将 WAL 缓冲区强制刷盘。
   *
   * 这是 WAL durability 的关键点：是否频繁调用它，决定了吞吐和崩溃安全之间的取舍。
   *
   * @return 成功返回 `RC::SUCCESS`，否则返回错误码。
   */
  RC sync() { return RC::UNIMPLEMENTED; }

  const string &filename() const { return filename_; }

private:
  // 当前活跃 WAL 文件名。Manifest 中的 memtable_id 会间接指向这里对应的日志文件。
  // 仅保存日志文件名；真正的写句柄未来可在实现中补齐。
  string filename_;
};
}  // namespace oceanbase
