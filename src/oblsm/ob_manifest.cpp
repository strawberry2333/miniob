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
#include "oblsm/ob_manifest.h"
#include "common/log/log.h"
#include "common/sys/rc.h"
#include "oblsm/util/ob_file_writer.h"

namespace oceanbase {

RC ObManifestSSTableInfo::from_json(const Json::Value &v)
{
  // 恢复时要求字段完整存在，否则宁可失败也不要接受损坏的元数据。
  if (v.isMember("sstable_id") && v["sstable_id"].isInt()) {
    sstable_id = v["sstable_id"].asInt();
  } else {
    return RC::JSON_MEMBER_MISSING;
  }

  if (v.isMember("level") && v["level"].isInt()) {
    level = v["level"].asInt();
  } else {
    return RC::JSON_MEMBER_MISSING;
  }
  return RC::SUCCESS;
}

Json::Value ObManifestCompaction::to_json() const
{
  // 一条 compaction record 完整描述“删了哪些表、加了哪些表”。
  Json::Value v;
  v["record_type"]     = Json::Value{ObManifestCompaction::record_type()};
  v["compaction_type"] = JsonConverter::to_json(compaction_type);
  Json::Value deleted(Json::arrayValue);
  for (const auto &table : deleted_tables) {
    deleted.append(JsonConverter::to_json(table));
  }
  v["deleted_tables"] = deleted;

  Json::Value added(Json::arrayValue);
  for (const auto &table : added_tables) {
    added.append(JsonConverter::to_json(table));
  }
  v["added_tables"] = added;

  v["sstable_sequence_id"] = static_cast<Json::UInt64>(sstable_sequence_id);

  v["seq_id"] = static_cast<Json::UInt64>(seq_id);
  return v;
}

RC ObManifestCompaction::from_json(const Json::Value &v)
{
  RC rc = RC::SUCCESS;

  // 先恢复 compaction 策略类型。
  if (!v.isMember("compaction_type")) {
    return RC::JSON_MEMBER_MISSING;
  }
  rc = JsonConverter::from_json<CompactionType>(v["compaction_type"], compaction_type);
  if (rc != RC::SUCCESS) {
    LOG_ERROR("Failed to convert CompactionType from manifest record, rc=%s", strrc(rc));
    return rc;
  }

  // 恢复本次 compaction 删除的 SSTable 集合。
  if (!v.isMember("deleted_tables") || !v["deleted_tables"].isArray()) {
    return RC::JSON_MEMBER_MISSING;
  }
  for (const auto &item : v["deleted_tables"]) {
    ObManifestSSTableInfo info;
    rc = JsonConverter::from_json<ObManifestSSTableInfo>(item, info);
    if (rc != RC::SUCCESS) {
      LOG_ERROR("Failed to convert ObManifestSSTableInfo from json, rc=%s", strrc(rc));
      return rc;
    }
    deleted_tables.push_back(std::move(info));
  }

  // 恢复本次 compaction 新增的 SSTable 集合。
  if (!v.isMember("added_tables") || !v["added_tables"].isArray()) {
    return RC::JSON_MEMBER_MISSING;
  }
  for (const auto &item : v["added_tables"]) {
    ObManifestSSTableInfo info;
    rc = JsonConverter::from_json<ObManifestSSTableInfo>(item, info);
    if (rc != RC::SUCCESS) {
      LOG_ERROR("Failed to convert ObManifestSSTableInfo from json, rc=%s", strrc(rc));
      return rc;
    }
    added_tables.push_back(std::move(info));
  }

  // 恢复当时全局的 SSTable 编号上界。
  if (v.isMember("sstable_sequence_id") && v["sstable_sequence_id"].isUInt64()) {
    sstable_sequence_id = v["sstable_sequence_id"].asUInt64();
  } else {
    return RC::JSON_MEMBER_MISSING;
  }

  // 恢复当时已经持久化的最新写序列号。
  if (v.isMember("seq_id") && v["seq_id"].isUInt64()) {
    seq_id = v["seq_id"].asUInt64();
  } else {
    return RC::JSON_MEMBER_MISSING;
  }

  return RC::SUCCESS;
}

Json::Value ObManifestSnapshot::to_json() const
{
  // 快照记录的是“一个时刻的全量视图”，不是增量变化。
  Json::Value root;
  root["record_type"] = Json::Value{ObManifestSnapshot::record_type()};
  root["seq"]         = seq;
  root["sstable_id"]  = sstable_id;

  root["compaction_type"] = static_cast<int>(compaction_type);

  Json::Value sstables_json(Json::arrayValue);
  for (const auto &level : sstables) {
    Json::Value level_array(Json::arrayValue);
    for (const auto &sst_id : level) {
      level_array.append(sst_id);
    }
    sstables_json.append(level_array);
  }
  root["sstables"] = sstables_json;
  return root;
}

RC ObManifestSnapshot::from_json(const Json::Value &v)
{
  if (!v.isMember("seq") || !v.isMember("sstable_id") || !v.isMember("compaction_type") || !v.isMember("sstables")) {
    return RC::JSON_MEMBER_MISSING;
  }
  seq             = v["seq"].asUInt64();
  sstable_id      = v["sstable_id"].asUInt64();
  compaction_type = static_cast<CompactionType>(v["compaction_type"].asInt());
  sstables.clear();
  for (const auto &level_json : v["sstables"]) {
    std::vector<uint64_t> level;
    for (const auto &sst_id_json : level_json) {
      level.push_back(sst_id_json.asUInt64());
    }
    sstables.push_back(level);
  }
  return RC::SUCCESS;
}

Json::Value ObManifestNewMemtable::to_json() const
{
  Json::Value root;
  root["record_type"] = ObManifestNewMemtable::record_type();
  root["memtable_id"] = memtable_id;
  return root;
}

RC ObManifestNewMemtable::from_json(const Json::Value &v)
{
  if (!v.isMember("memtable_id")) {
    return RC::JSON_MEMBER_MISSING;
  }
  memtable_id = v["memtable_id"].asUInt64();
  return RC::SUCCESS;
}

RC ObManifest::open()
{
  string current_path_str = current_path_.generic_string();
  if (filesystem::exists(current_path_)) {
    // 已存在 CURRENT，说明目录里曾经初始化过 LSM，先找到当前生效的 Manifest。
    reader_ = ObFileReader::create_file_reader(current_path_str);
    RC rc   = reader_->open_file();
    if (rc != RC::SUCCESS) {
      return RC::IOERR_OPEN;
    }

    string manifest_file_id = reader_->read_pos(0, reader_->file_size());
    mf_seq_                 = atoi(manifest_file_id.c_str());
    string mf_file          = get_manifest_file_path(path_, mf_seq_);
    reader_->close_file();

    reader_ = ObFileReader::create_file_reader(mf_file);
    writer_ = ObFileWriter::create_file_writer(mf_file, true);
    if (!reader_) {
      LOG_ERROR("Failed to create manifest reader %s", mf_file.c_str());
      return RC::IOERR_OPEN;
    }
    if (!writer_) {
      LOG_ERROR("Failed to create manifest writer %s", mf_file.c_str());
      return RC::IOERR_OPEN;
    }

  } else {
    // 首次初始化目录：创建 CURRENT 和第一个空 Manifest。
    mf_seq_                  = 0;
    string current_file_data = std::to_string(mf_seq_);
    string mf_file           = get_manifest_file_path(path_, mf_seq_);

    writer_ = ObFileWriter::create_file_writer(current_path_str, false);
    if (writer_ == nullptr) {
      LOG_ERROR("Failed to open manifest writer %s", mf_file.c_str());
      return RC::IOERR_OPEN;
    }

    writer_->write(string_view{current_file_data});
    writer_->flush();
    writer_->close_file();

    writer_ = ObFileWriter::create_file_writer(mf_file, false);
    reader_ = ObFileReader::create_file_reader(mf_file);
    if (!reader_) {
      LOG_ERROR("Failed to open manifest reader %s", mf_file.c_str());
      return RC::IOERR_OPEN;
    }
    if (!writer_) {
      LOG_ERROR("Failed to open manifest writer %s", mf_file.c_str());
      return RC::IOERR_OPEN;
    }

    // 初始化时默认活跃 memtable_id 为 0，对应后续第一个 WAL。
    ObManifestNewMemtable memtable_record;
    memtable_record.memtable_id = 0;
    RC rc                       = push(memtable_record);
    if (OB_FAIL(rc)) {
      LOG_WARN("Failed to push ObManifestNewMemtable record into manifest file %s", strrc(rc));
      return rc;
    }
  }

  return RC::SUCCESS;
}

RC ObManifest::recover(std::unique_ptr<ObManifestSnapshot> &snapshot_record,
    std::unique_ptr<ObManifestNewMemtable> &memtbale_record, std::vector<ObManifestCompaction> &records)
{
  size_t               len       = 0;
  uint32_t             pos       = 0;
  uint32_t             file_size = 0;
  ObManifestCompaction compaction_record;
  RC                   rc = RC::SUCCESS;

  rc        = reader_->open_file();
  file_size = reader_->file_size();
  if (OB_FAIL(rc)) {
    LOG_ERROR("Failed to open manifest file");
    return rc;
  }

  // Manifest 是顺序 append 的，恢复时按记录顺序从头扫到尾即可。
  while (file_size > pos) {
    string len_str = reader_->read_pos(pos, sizeof(len));
    memcpy(&len, len_str.data(), sizeof(len));
    pos += sizeof(len);
    ASSERT(len > 0, "len %d should be greater than 0", len);
    string json_raw = reader_->read_pos(pos, static_cast<uint32_t>(len));
    pos += json_raw.size();
    // 先把一条完整 JSON 记录读出来，再根据 record_type 分发到不同对象。
    Json::Reader reader;
    Json::Value  json_val;
    bool         ok = reader.parse(json_raw, json_val);
    if (!ok) {
      return RC::JSON_PARSE_FAILED;
    }

    if (!json_val.isMember("record_type")) {
      return RC::JSON_MEMBER_MISSING;
    }

    string record_type = json_val["record_type"].asString();
    if (record_type == ObManifestCompaction::record_type()) {
      rc = JsonConverter::from_json(json_val, compaction_record);
      if (rc != RC::SUCCESS) {
        LOG_ERROR("Failed to convert from JSON to compaction record. JSON %s", json_val.toStyledString().c_str());
        return rc;
      }
      // compaction record 需要按原始顺序保留，后续统一回放。
      records.emplace_back(std::move(compaction_record));
    } else if (record_type == ObManifestNewMemtable::record_type()) {
      // 只关心最后一条 NewMemtable 记录，因为它代表当前活跃 WAL。
      memtbale_record = std::make_unique<ObManifestNewMemtable>();
      rc              = JsonConverter::from_json(json_val, *memtbale_record);
      if (rc != RC::SUCCESS) {
        LOG_ERROR("Failed to convert from JSON to new memtbale record. JSON %s", json_val.toStyledString().c_str());
        return rc;
      }
    } else if (record_type == ObManifestSnapshot::record_type()) {
      // 同理，后出现的 snapshot 会覆盖之前的快照。
      snapshot_record = std::make_unique<ObManifestSnapshot>();
      rc              = JsonConverter::from_json(json_val, *snapshot_record);
      if (rc != RC::SUCCESS) {
        LOG_ERROR("Failed to convert from JSON to snapshot record. JSON %s", json_val.toStyledString().c_str());
        return rc;
      }
    } else {
      return RC::VARIABLE_NOT_VALID;
    }
  }
  return rc;
}

RC ObManifest::redirect(const ObManifestSnapshot &snapshot, const ObManifestNewMemtable &memtable)
{

  auto   prev_mf_file = get_manifest_file_path(path_, mf_seq_++);
  auto   new_mf_file  = get_manifest_file_path(path_, mf_seq_);
  string current_file = path_ / "CURRENT";
  RC     rc           = RC::SUCCESS;
  // 先把新 Manifest 文件写完整，再切换 CURRENT，避免中途崩溃导致 CURRENT 指向坏文件。
  writer_ = ObFileWriter::create_file_writer(new_mf_file, false);
  if (!writer_) {
    return RC::IOERR_OPEN;
  }
  rc = push(snapshot);
  if (OB_FAIL(rc)) {
    LOG_ERROR("Failed to push ObManifestSnapshot record into manifest file");
    return rc;
  }

  rc = push(memtable);
  if (OB_FAIL(rc)) {
    LOG_ERROR("Failed to push ObManifestNewMemtable record into manifest file");
    return rc;
  }

  // CURRENT 是一个极小的“指针文件”，只存当前生效的 manifest 编号。
  auto current_writer = ObFileWriter::create_file_writer(current_file, false);
  rc                  = current_writer->write(std::to_string(mf_seq_));
  if (OB_FAIL(rc)) {
    LOG_ERROR("Failed to write mf_seq_ into CURRENT file");
    return rc;
  }

  rc = current_writer->flush();
  if (OB_FAIL(rc)) {
    LOG_ERROR("Failed to flush data into CURRENT file");
    return rc;
  }
  current_writer->close_file();

  // 新文件切换成功后，旧 Manifest 即可删除。
  if (filesystem::exists(prev_mf_file)) {
    remove(prev_mf_file.c_str());
  }

  return rc;
}

}  // namespace oceanbase
