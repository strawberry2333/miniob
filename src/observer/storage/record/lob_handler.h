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

#include "common/lang/mutex.h"
#include "common/lang/sstream.h"
#include "common/types.h"
#include "storage/persist/persist.h"

/**
 * @file lob_handler.h
 * @brief 定义独立 LOB 文件的顺序追加与读取封装。
 */

/**
 * @brief 管理LOB文件中的 LOB 对象
 * @ingroup RecordManager
 * @details 当前实现把每个 LOB 当作一段追加写入的数据块，通过偏移量回查内容。
 */
class LobFileHandler
{
public:
  LobFileHandler() {}

  ~LobFileHandler() { close_file(); }

  /// @brief 创建一个新的 LOB 文件并绑定到底层 `PersistHandler`。
  RC create_file(const char *file_name);

  /// @brief 打开一个已有 LOB 文件。
  RC open_file(const char *file_name);

  RC close_file() { return file_.close_file(); }

  /**
   * @brief 追加一段 LOB 数据并返回其起始偏移。
   * @details 写入不足会返回 `IOERR_WRITE`，调用方需要自行决定是否回滚元数据引用。
   */
  RC insert_data(int64_t &offset, int64_t length, const char *data);

  RC get_data(int64_t offset, int64_t length, char *data) { return file_.read_at(offset, length, data); }

private:
  PersistHandler file_;
};
