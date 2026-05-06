/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
   miniob is licensed under Mulan PSL v2.
   You can use this software according to the terms and conditions of the Mulan PSL v2.
   You may obtain a copy of Mulan PSL v2 at:
            http://license.coscl.org.cn/MulanPSL2
   THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
   EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
   MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
   See the Mulan PSL v2 for more details. */
//
// Created by Ping Xu(haibarapink@gmail.com) on 2025/1/24.
//
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
   * @brief Constructor to initialize with given SSTable ID and level.
   *
   * @param sstable_id_ The SSTable ID.
   * @param level_ The SSTable level.
   */
  ObManifestSSTableInfo(int sstable_id_, int level_) : sstable_id(sstable_id_), level(level_) {}

  /** Default constructor with default values (0). */
  ObManifestSSTableInfo() : sstable_id(0), level(0) {}

  /**
   * @brief Equality operator for comparing two `ObManifestSSTableInfo` objects.
   *
   * @param rhs The other `ObManifestSSTableInfo` to compare with.
   * @return `true` if the two objects are equal, `false` otherwise.
   */
  bool operator==(const ObManifestSSTableInfo &rhs) const { return sstable_id == rhs.sstable_id && level == rhs.level; }

  /**
   * @brief Converts the `ObManifestSSTableInfo` object to a JSON value.
   *
   * @return The JSON value representing the SSTable info.
   */
  Json::Value to_json() const
  {
    Json::Value v;
    v["sstable_id"] = sstable_id;
    v["level"]      = level;
    return v;
  }

  /**
   * @brief Populates the `ObManifestSSTableInfo` object from a JSON value.
   *
   * @param v The JSON value to populate the object with.
   * @return An RC indicating the result of the operation.
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
  CompactionType                     compaction_type;
  std::vector<ObManifestSSTableInfo> deleted_tables;
  std::vector<ObManifestSSTableInfo> added_tables;
  uint64_t                           sstable_sequence_id;
  uint64_t                           seq_id;

  static string record_type() { return "ObManifestCompaction"; }

  bool operator==(const ObManifestCompaction &rhs) const
  {
    return compaction_type == rhs.compaction_type && deleted_tables == rhs.deleted_tables &&
           added_tables == rhs.added_tables && sstable_sequence_id == rhs.sstable_sequence_id && seq_id == rhs.seq_id;
  }

  /**
   * @brief Converts the `ObManifestCompaction` to a JSON value.
   *
   * @return The JSON value representing the record.
   */
  Json::Value to_json() const;

  /**
   * @brief Populates the `ObManifestCompaction` object from a JSON value.
   *
   * @param v The JSON value to populate the object with.
   * @return An RC indicating the result of the operation.
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
  std::vector<std::vector<uint64_t>> sstables;
  uint64_t                           seq;
  uint64_t                           sstable_id;
  CompactionType                     compaction_type;

  static string record_type() { return "ObManifestSnapshot"; }

  bool operator==(const ObManifestSnapshot &rhs) const
  {
    return sstables == rhs.sstables && seq == rhs.seq && sstable_id == rhs.sstable_id &&
           compaction_type == rhs.compaction_type;
  }

  /**
   * @brief Converts the `ObManifestSnapshot` to a JSON value.
   *
   * @return The JSON value representing the snapshot.
   */
  Json::Value to_json() const;

  /**
   * @brief Populates the `ObManifestSnapshot` object from a JSON value.
   *
   * @param v The JSON value to populate the object with.
   * @return An RC indicating the result of the operation.
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
  uint64_t memtable_id;

  static string record_type() { return "ObManifestNewMemtable"; }

  bool operator==(const ObManifestNewMemtable &rhs) const { return memtable_id == rhs.memtable_id; }

  /**
   * @brief Converts the `ObManifestNewMemtable` to a JSON value.
   *
   * @return The JSON value representing the new memtable.
   */
  Json::Value to_json() const;

  /**
   * @brief Populates the `ObManifestNewMemtable` object from a JSON value.
   *
   * @param v The JSON value to populate the object with.
   * @return An RC indicating the result of the operation.
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
 */
class ObManifest
{
public:
  /**
   * @brief 基于数据目录构造 Manifest 管理器。
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
   */
  RC redirect(const ObManifestSnapshot &snapshot, const ObManifestNewMemtable &memtable);

  /**
   * @brief 顺序读取当前 Manifest 文件中的所有记录，用于恢复。
   */
  RC recover(std::unique_ptr<ObManifestSnapshot> &snapshot_record,
      std::unique_ptr<ObManifestNewMemtable> &memtbale_record, std::vector<ObManifestCompaction> &compactions);

  // 当前已经持久化到元数据中的最新序列号。
  uint64_t latest_seq{0};

  friend class ObManifestTester;

private:
  /**
   * @brief 生成某个 Manifest 文件的路径。
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
