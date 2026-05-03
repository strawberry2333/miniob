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

#include "sql/stmt/load_data_stmt.h"
#include "common/lang/string.h"
#include "common/log/log.h"
#include "storage/db/db.h"
#include <unistd.h>

using namespace common;

/**
 * @file load_data_stmt.cpp
 * @brief 实现 `LOAD DATA` 的语义绑定和基础参数校验。
 */

RC LoadDataStmt::create(Db *db, const LoadDataSqlNode &load_data, Stmt *&stmt)
{
  RC rc = RC::SUCCESS;

  const char *table_name = load_data.relation_name.c_str();
  if (is_blank(table_name) || is_blank(load_data.file_name.c_str())) {
    LOG_WARN("invalid argument. db=%p, table_name=%p, file name=%s",
        db, table_name, load_data.file_name.c_str());
    return RC::INVALID_ARGUMENT;
  }

  // check whether the table exists
  // 第 1 步：把目标表名绑定到具体表对象。
  Table *table = db->find_table(table_name);
  if (nullptr == table) {
    LOG_WARN("no such table. db=%s, table_name=%s", db->name(), table_name);
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  // 第 2 步：在 resolve 阶段就提前验证文件存在且可读，避免执行阶段再晚失败。
  if (0 != access(load_data.file_name.c_str(), R_OK)) {
    LOG_WARN("no such file to load. file name=%s, error=%s", load_data.file_name.c_str(), strerror(errno));
    return RC::FILE_NOT_EXIST;
  }

  // 第 3 步：当前语法层会把引号也保留下来，因此这里按既有约定检查长度是否为 3。
  if (load_data.enclosed.size() != 3) {
    LOG_WARN("load data invalid enclosed. enclosed=%s", load_data.enclosed.c_str());
    return RC::INVALID_ARGUMENT;
  }
  if (load_data.terminated.size() != 3) {
    LOG_WARN("load data invalid terminated. terminated=%s", load_data.terminated.c_str());
    return RC::INVALID_ARGUMENT;
  }

  // 第 4 步：提取真正的单字符分隔符/包裹符，构造最终 stmt。
  stmt = new LoadDataStmt(table, load_data.file_name.c_str(), load_data.terminated[1], load_data.enclosed[1]);
  return rc;
}
