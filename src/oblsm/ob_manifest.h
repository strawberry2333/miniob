/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
   miniob is licensed under Mulan PSL v2.
   You can use this software according to the terms and conditions of the Mulan PSL v2.
   You may obtain a copy of Mulan PSL v2 at:
            http://license.coscl.org.cn/MulanPSL2
   THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
   EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
   MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
   See the Mulan PSL v2 for more details. */
// Manifest 元数据定义与管理接口。
// 该文件描述 oblsm 如何把“当前哪些 SSTable 有效、最新序列号是多少、
// 当前活跃 WAL 属于哪个 memtable”等恢复所需信息持久化到磁盘。
#pragma once

#include "json/json.h"
#include "common/lang/filesystem.h"
#include "common/lang/string_view.h"
#include "common/log/log.h"
#include "common/sys/rc.h"
#include "oblsm/ob_lsm_define.h"
#include "oblsm/util/ob_file_writer.h"
#include "oblsm/util/ob_file_reader.h"

namespace oceanbase {

/**
 * @brief JSON 与 Manifest 记录对象之间的转换工具。
 *
 * Manifest 文件本质上是一串“带长度前缀的 JSON 记录”，因此所有记录类型都需要
 * 能在内存对象和 JSON 之间来回转换。
 */
class JsonConverter
{
public:
  /**
   * @brief 把业务对象序列化成 JSON。
   */
  template <typename T>
  static Json::Value to_json(const T &t)
  {
    return t.to_json();
  }

  /**
   * @brief 把 JSON 反序列化回业务对象。
   */
  template <typename T>
  static RC from_json(const Json::Value &v, T &t)
  {
    RC rc = t.from_json(v);
    return rc;
  }
};

/**
 * @brief `CompactionType` 的 JSON 序列化特化。
 */
template <>
inline Json::Value JsonConverter::to_json<CompactionType>(const CompactionType &type)
{
  int         type_as_int = static_cast<int>(type);
  Json::Value res{type_as_int};
  return res;
}

/**
 * @brief `CompactionType` 的 JSON 反序列化特化。
 */
template <>
inline RC JsonConverter::from_json<CompactionType>(const Json::Value &v, CompactionType &type)
{
  int type_as_int = -1;
  if (v.isInt()) {
    type_as_int = v.asInt();
  } else {
    return RC::INVALID_ARGUMENT;
  }

  type = static_cast<CompactionType>(type_as_int);
  return RC::SUCCESS;
}

/**
 * @brief Manifest 中描述某个 SSTable 所在位置的元信息。
 *
 * 这里不直接保存整个 SSTable 的细节，只保存：
 * - sstable_id：文件身份
 * - level：它属于哪个层级/哪个 run
 */
struct ObManifestSSTableInfo
{
  int sstable_id;
  int level;

  /**
   * @brief 用 `(sstable_id, level)` 初始化一条 SSTable 位置信息。
   *
   * 这里的 `level` 不是“压缩后的目标层级”一类临时概念，
   * 而是这张表在 Manifest 视角下所属的稳定层级，用来在恢复时
   * 把 SSTable 放回正确的位置。
   */
  ObManifestSSTableInfo(int sstable_id_, int level_) : sstable_id(sstable_id_), level(level_) {}

  /** @brief 默认构造，便于先创建对象再做 JSON 反序列化。 */
  ObManifestSSTableInfo() : sstable_id(0), level(0) {}

  /**
   * @brief 判断两条 SSTable 元信息是否表示同一张表。
   *
   * Manifest 里的“删除某张表”语义依赖这个比较：
   * 只有 `sstable_id` 和 `level` 都一致，才认为是同一条元数据记录。
   */
  bool operator==(const ObManifestSSTableInfo &rhs) const { return sstable_id == rhs.sstable_id && level == rhs.level; }

  /**
   * @brief 序列化为 JSON。
   *
   * 只保存恢复所需的最小集合，不携带布隆过滤器、索引块等运行时细节。
   */
  Json::Value to_json() const
  {
    Json::Value v;
    v["sstable_id"] = sstable_id;
    v["level"]      = level;
    return v;
  }

  /**
   * @brief 从 JSON 反序列化出 SSTable 位置信息。
   */
  RC from_json(const Json::Value &v);
};

/**
 * @brief 一条 compaction 变更记录。
 *
 * 这类记录描述的是“某次 compaction 之后 SSTable 集合发生了哪些变化”，
 * 恢复时只要按顺序回放这些增删操作，就能还原出当时的磁盘视图。
 */
class ObManifestCompaction
{
public:
  // 本次 compaction 采用的策略类型。恢复磁盘视图时本字段主要用于保留历史语义，
  // 真实回放依赖 deleted_tables / added_tables 这两个集合。
  CompactionType                     compaction_type;
  // 本次 compaction 成功提交后，不再属于当前 LSM 视图的旧 SSTable。
  std::vector<ObManifestSSTableInfo> deleted_tables;
  // 本次 compaction 产出的新 SSTable。恢复时需要把它们加入对应层级。
  std::vector<ObManifestSSTableInfo> added_tables;
  // 记录写入时系统已分配到的最新 SSTable 编号，用于恢复后续编号生成器。
  uint64_t                           sstable_sequence_id;
  // 记录写入时已经持久化的最大写序列号，帮助恢复全局 seq 单调递增。
  uint64_t                           seq_id;

  static string record_type() { return "ObManifestCompaction"; }

  bool operator==(const ObManifestCompaction &rhs) const
  {
    return compaction_type == rhs.compaction_type && deleted_tables == rhs.deleted_tables &&
           added_tables == rhs.added_tables && sstable_sequence_id == rhs.sstable_sequence_id && seq_id == rhs.seq_id;
  }

  /**
   * @brief 把一次 compaction 提交结果编码成 Manifest 增量记录。
   *
   * 这条记录的语义是“状态变更日志”而不是“执行计划”：
   * 即使运行时如何选表、如何 merge 发生变化，只要最终增删集合一致，
   * 恢复出来的 SSTable 视图就一致。
   */
  Json::Value to_json() const;

  /**
   * @brief 从 Manifest 里的增量记录恢复 compaction 结果。
   */
  RC from_json(const Json::Value &v);
};

/**
 * @brief 一条全量快照记录。
 *
 * 它直接描述某一时刻整个 LSM 的 SSTable 拓扑与关键序列号，
 * 用于加速恢复，避免每次启动都从最早的 compaction record 一直回放。
 */
class ObManifestSnapshot
{
public:
  // 每个下标对应一个 level/run，里面保存该层所有 SSTable 的编号。
  std::vector<std::vector<uint64_t>> sstables;
  // 生成快照时系统认可的最新写序列号。
  uint64_t                           seq;
  // 生成快照时系统认可的最新 SSTable 编号。
  uint64_t                           sstable_id;
  // 快照生成时采用的 compaction 模式，恢复后需要按相同策略继续运行。
  CompactionType                     compaction_type;

  static string record_type() { return "ObManifestSnapshot"; }

  bool operator==(const ObManifestSnapshot &rhs) const
  {
    return sstables == rhs.sstables && seq == rhs.seq && sstable_id == rhs.sstable_id &&
           compaction_type == rhs.compaction_type;
  }

  /**
   * @brief 序列化完整快照。
   *
   * 快照的作用是截断历史：启动时优先读取最后一份 snapshot，
   * 再回放其后的 compaction 记录，而不是永远从第一条日志开始恢复。
   */
  Json::Value to_json() const;

  /**
   * @brief 从 JSON 中恢复一份全量快照。
   */
  RC from_json(const Json::Value &v);
};

/**
 * @brief 记录“当前活跃 memtable 对应的 WAL 编号”。
 *
 * 启动恢复时，磁盘上的 SSTable 状态可以由快照和 compaction record 还原，
 * 而最新尚未刷盘的内存写入，则需要靠这条记录定位 WAL 后再重放。
 */
class ObManifestNewMemtable
{
public:
  // 当前活跃 memtable 的编号。实现上它也承担“当前活跃 WAL 文件编号”的定位职责。
  uint64_t memtable_id;

  static string record_type() { return "ObManifestNewMemtable"; }

  bool operator==(const ObManifestNewMemtable &rhs) const { return memtable_id == rhs.memtable_id; }

  /**
   * @brief 序列化当前活跃 memtable/WAL 的标识。
   */
  Json::Value to_json() const;

  /**
   * @brief 从 JSON 中恢复当前活跃 memtable/WAL 的标识。
   */
  RC from_json(const Json::Value &v);
};

/**
 * @brief Manifest 文件管理器。
 *
 * Manifest 是 ObLsm 的“元数据日志”：
 * - 记录当前有哪些 SSTable；
 * - 记录每次 compaction 新增/删除了哪些 SSTable；
 * - 记录最新 memtable/WAL 的编号；
 * - 负责在恢复后生成新的 snapshot，压缩历史日志长度。
 *
 * 可以把它理解成“磁盘状态的真相来源”：
 * SSTable 文件本身只负责存数据，Manifest 才负责声明“哪些文件当前属于有效 LSM 视图”。
 */
class ObManifest
{
public:
  /**
   * @brief 基于数据目录构造 Manifest 管理器。
   *
   * 目录中会维护：
   * - `CURRENT`：一个很小的指针文件，只保存当前 Manifest 序号；
   * - `*.manifest`：真正的元数据日志文件。
   */
  ObManifest(const std::string &path) : path_(filesystem::path(path)) { current_path_ = path_ / "CURRENT"; }

  /**
   * @brief 打开 CURRENT 指向的 Manifest 文件。
   *
   * 若当前目录还没有 Manifest，则会初始化一个空的 Manifest 和默认的
   * `ObManifestNewMemtable` 记录。
   */
  RC open();

  /**
   * @brief 追加一条 Manifest 记录。
   *
   * 落盘格式为：
   * `| json_length(size_t) | json_payload |`
   *
   * 之所以带长度前缀，是为了恢复时可以顺序扫描，不依赖换行或分隔符。
   * 每次 `push()` 都会在记录末尾主动 flush，确保 CURRENT 仍指向该文件期间，
   * 文件尾部始终是“完整记录边界”，不把半条 JSON 暴露给恢复流程。
   */
  template <typename T>
  RC push(const T &data)
  {
    static_assert(std::is_same<T, ObManifestCompaction>::value || std::is_same<T, ObManifestSnapshot>::value ||
                      std::is_same<T, ObManifestNewMemtable>::value,
        "push() only supports ObManifestCompaction ,ObManifestNewMemtable and ObManifestSnapshot types.");
    Json::Value json = JsonConverter::to_json(data);
    string      str  = json.toStyledString();
    size_t      len  = str.size();
    RC          rc   = writer_->write(string_view{reinterpret_cast<char *>(&len), sizeof(len)});
    if (OB_FAIL(rc)) {
      LOG_WARN("Failed to push record into manifest file %s, rc %s", writer_->file_name().c_str(), strrc(rc));
      return rc;
    }
    rc = writer_->write(str);
    if (OB_FAIL(rc)) {
      LOG_WARN("Failed to push record into manifest file %s,  rc %s", writer_->file_name().c_str(), strrc(rc));
      return rc;
    }
    rc = writer_->flush();
    if (OB_FAIL(rc)) {
      LOG_WARN("Failed to flush data of manifest file %s, rc %s", writer_->file_name().c_str(), strrc(rc));
      return rc;
    }
    return rc;
  }

  /**
   * @brief 切换到新的 Manifest 文件，并写入最新 snapshot 与 memtable 记录。
   *
   * 这个动作常发生在恢复后或需要压缩历史时。切换成功之后：
   * - 旧 Manifest 可以删除；
   * - 新 Manifest 从一份短快照重新开始累计增量。
   */
  RC redirect(const ObManifestSnapshot &snapshot, const ObManifestNewMemtable &memtable);

  /**
   * @brief 顺序读取当前 Manifest 文件中的所有记录，用于恢复。
   *
   * 恢复端通常按以下顺序使用返回结果：
   * 1. 先取最后一份 `snapshot_record` 还原磁盘 SSTable 视图；
   * 2. 再按顺序回放 `compactions`，补上快照之后的增量变化；
   * 3. 最后读取 `memtbale_record`，定位需要继续重放的活跃 WAL。
   */
  RC recover(std::unique_ptr<ObManifestSnapshot> &snapshot_record,
      std::unique_ptr<ObManifestNewMemtable> &memtbale_record, std::vector<ObManifestCompaction> &compactions);

  // 当前已经持久化到元数据中的最新序列号。
  // 该字段不是从文件自动维护出来的，而是由上层在关键落盘点（如 freeze / compaction）
  // 主动更新，随后写入 compaction record 或 snapshot。
  uint64_t latest_seq{0};

  friend class ObManifestTester;

private:
  /**
   * @brief 生成某个 Manifest 文件的路径。
   *
   * 同一个数据目录下允许存在多份历史 Manifest，但任一时刻只有
   * `CURRENT` 指向的那一份被视为有效元数据。
   */
  string get_manifest_file_path(string path, uint64_t mf_seq)
  {
    return filesystem::path(path) / (std::to_string(mf_seq) + MANIFEST_SUFFIX);
  }

private:
  filesystem::path path_;
  filesystem::path current_path_;
  uint64_t         mf_seq_;

  std::unique_ptr<ObFileWriter> writer_;
  std::unique_ptr<ObFileReader> reader_;
};

}  // namespace oceanbase
