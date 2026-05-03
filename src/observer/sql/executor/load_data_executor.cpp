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
// Created by Wangyunlai on 2023/7/12.
//

#include "sql/executor/load_data_executor.h"
#include "common/lang/string.h"
#include "event/session_event.h"
#include "event/sql_event.h"
#include "sql/executor/sql_result.h"
#include "sql/stmt/load_data_stmt.h"
#include "storage/common/chunk.h"

using namespace common;

/**
 * @file load_data_executor.cpp
 * @brief 实现批量导入文件到表的执行逻辑。
 */

RC LoadDataExecutor::execute(SQLStageEvent *sql_event)
{
  RC            rc         = RC::SUCCESS;
  SqlResult    *sql_result = sql_event->session_event()->sql_result();
  LoadDataStmt *stmt       = static_cast<LoadDataStmt *>(sql_event->stmt());
  Table        *table      = stmt->table();
  const char   *file_name  = stmt->filename();
  // `LoadDataStmt` 已经完成目标表与文件参数解析，这里只负责真正的导入流程。
  load_data(table, file_name, stmt->terminated(), stmt->enclosed(), sql_result);
  return rc;
}

/**
 * 从文件中导入数据时使用。尝试向表中插入解析后的一行数据。
 * @param table  要导入的表
 * @param file_values 从文件中读取到的一行数据，使用分隔符拆分后的几个字段值
 * @param record_values Table::insert_record使用的参数，为了防止频繁的申请内存
 * @param errmsg 如果出现错误，通过这个参数返回错误信息
 * @return 成功返回RC::SUCCESS
 */
RC insert_record_from_file(
    Table *table, vector<string> &file_values, vector<Value> &record_values, stringstream &errmsg)
{
  const int field_num     = record_values.size();
  const int sys_field_num = table->table_meta().sys_field_num();

  // 文件列数不足以填满表字段时，直接报 schema mismatch。
  if (file_values.size() < record_values.size()) {
    return RC::SCHEMA_FIELD_MISSING;
  }

  RC rc = RC::SUCCESS;

  // 先把文本列逐一转换成对应类型的 `Value`，后续再统一拼装成 record。
  for (int i = 0; i < field_num && RC::SUCCESS == rc; i++) {
    const FieldMeta *field = table->table_meta().field(i + sys_field_num);

    string &file_value = file_values[i];
    // 非字符串列沿用现有实现先做 strip，兼容文件中常见的空白字符。
    if (field->type() != AttrType::CHARS) {
      common::strip(file_value);
    }
    rc = DataType::type_instance(field->type())->set_value_from_str(record_values[i], file_value);
    if (rc != RC::SUCCESS) {
      LOG_WARN("Failed to deserialize value from string: %s, type=%d", file_value.c_str(), field->type());
      return rc;
    }
  }

  if (RC::SUCCESS == rc) {
    Record record;
    // 先构造存储层 record，再真正插入到表中。
    rc = table->make_record(field_num, record_values.data(), record);
    if (rc != RC::SUCCESS) {
      errmsg << "insert failed.";
    } else if (RC::SUCCESS != (rc = table->insert_record(record))) {
      errmsg << "insert failed.";
    }
  }
  return rc;
}


// TODO: pax format and row format
void LoadDataExecutor::load_data(Table *table, const char *file_name, char terminated, char enclosed, SqlResult *sql_result)
{
  // `terminated` / `enclosed` 已经通过接口传入，当前实现仍保留原有按行和固定分隔符拆分的行为。
  stringstream result_string;

  fstream fs;
  fs.open(file_name, ios_base::in | ios_base::binary);
  if (!fs.is_open()) {
    result_string << "Failed to open file: " << file_name << ". system error=" << strerror(errno) << endl;
    sql_result->set_return_code(RC::FILE_NOT_EXIST);
    sql_result->set_state_string(result_string.str());
    return;
  }

  struct timespec begin_time;
  clock_gettime(CLOCK_MONOTONIC, &begin_time);
  const int sys_field_num = table->table_meta().sys_field_num();
  const int field_num     = table->table_meta().field_num() - sys_field_num;

  vector<Value>  record_values(field_num);
  string         line;
  vector<string> file_values;
  const string   delim("|");
  int            line_num        = 0;
  int            insertion_count = 0;
  RC             rc              = RC::SUCCESS;
  while (!fs.eof() && RC::SUCCESS == rc) {
    getline(fs, line);
    line_num++;
    if (common::is_blank(line.c_str())) {
      continue;
    }

    // 现有实现按固定 `|` 分列，与解析层保留的参数设置保持兼容但尚未完全打通。
    file_values.clear();
    common::split_string(line, delim, file_values);
    stringstream errmsg;

    if (table->table_meta().storage_format() == StorageFormat::ROW_FORMAT) {
      // 行存格式按“文本列 -> Value -> Record -> insert”的路径逐条导入。
      rc = insert_record_from_file(table, file_values, record_values, errmsg);
      if (rc != RC::SUCCESS) {
        result_string << "Line:" << line_num << " insert record failed:" << errmsg.str() << ". error:" << strrc(rc)
                      << endl;
      } else {
        insertion_count++;
      }
    } else if (table->table_meta().storage_format() == StorageFormat::PAX_FORMAT) {
      // your code here
      // Todo: 参照insert_record_from_file实现
      rc = RC::UNIMPLEMENTED;
    } else {
      // 未知存储格式由执行层直接终止导入。
      rc = RC::UNSUPPORTED;
      result_string << "Unsupported storage format: " << strrc(rc) << endl;
    }
  }
  fs.close();

  struct timespec end_time;
  clock_gettime(CLOCK_MONOTONIC, &end_time);
  if (RC::SUCCESS == rc) {
    result_string << strrc(rc);
  }
  // 保持现有行为：日志里记录导入结果，对外返回码固定写入 `SqlResult`。
  LOG_INFO("load data done. row num: %s, result: %s", insertion_count, strrc(rc));
  sql_result->set_return_code(RC::SUCCESS);
}
