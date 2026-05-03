/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "storage/record/lob_handler.h"

/**
 * @file lob_handler.cpp
 * @brief LOB 文件顺序追加与打开实现。
 */

RC LobFileHandler::create_file(const char *file_name)
{
  return file_.create_file(file_name);
}

RC LobFileHandler::open_file(const char *file_name)
{
  std::ifstream file(file_name);
  if (file.good()) {
    // 先用 ifstream 探测存在性，避免 PersistHandler 把“不存在”和“打开失败”混成同一错误码。
    return file_.open_file(file_name);
  } else {
    return RC::FILE_NOT_EXIST;
  }
  return RC::INTERNAL;
}

RC LobFileHandler::insert_data(int64_t &offset, int64_t length, const char *data)
{
  RC       rc         = RC::SUCCESS;
  int64_t  out_size   = 0;
  int64_t end_offset = 0;
  rc                  = file_.append(length, data, &out_size, &end_offset);
  if (OB_FAIL(rc)) {
    return rc;
  }
  if (out_size != length) {
    // LOB 写入必须整块成功，否则 offset 已暴露给上层会导致后续读取到截断数据。
    return RC::IOERR_WRITE;
  }
  offset = end_offset;

  return rc;
}
