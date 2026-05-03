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
// Created by qiling on 2021/4/13.
//
#pragma once

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "common/sys/rc.h"
#include "common/lang/string.h"

/**
 * @file persist.h
 * @brief 提供绑定单文件的 POSIX 持久化读写封装。
 */

/**
 * @brief 单文件持久化辅助类。
 * @details 该类把“当前绑定文件名 + 当前文件描述符”封装在一起，便于上层模块统一处理顺序读写、
 * 定位读写和文件删除。约束如下：
 * 1. 一个 `PersistHandler` 同一时刻只绑定一个文件；
 * 2. 不提供内部并发保护，调用方需要串行化访问；
 * 3. 所有错误都会通过 `RC` 返回，不抛异常。
 */
class PersistHandler
{
public:
  PersistHandler();
  ~PersistHandler();

  /**
   * @brief 创建并绑定一个新文件。
   * @details 如果对象已绑定其它文件、文件名为空或目标文件已存在，会直接返回错误。
   */
  RC create_file(const char *file_name);

  /**
   * @brief 打开一个已有文件并绑定到当前对象。
   * @param file_name 为空时表示重新打开当前已绑定文件。
   */
  RC open_file(const char *file_name = nullptr);

  /**
   * @brief 关闭当前文件描述符，但保留文件名绑定关系。
   */
  RC close_file();

  /**
   * @brief 删除指定文件或当前已绑定文件。
   * @details 删除当前文件时，会先尝试关闭文件描述符，避免仍持有打开句柄。
   */
  RC remove_file(const char *file_name = nullptr);

  /**
   * @brief 从当前文件偏移位置顺序写入数据。
   * @details 该接口不负责重试短写，实际写入字节数会通过 `out_size` 返回给调用方判断。
   */
  RC write_file(int size, const char *data, int64_t *out_size = nullptr);

  /**
   * @brief 先定位再写入数据。
   * @details `lseek` 失败或写入不足都会返回错误，并保持调用方可观测的错误路径。
   */
  RC write_at(uint64_t offset, int size, const char *data, int64_t *out_size = nullptr);

  /**
   * @brief 追加写入数据到文件末尾。
   * @param out_offset 若非空，返回本次追加开始时的文件尾偏移。
   */
  RC append(int size, const char *data, int64_t *out_size = nullptr, int64_t *out_offset = nullptr);

  /**
   * @brief 从当前文件偏移位置顺序读取数据。
   */
  RC read_file(int size, char *data, int64_t *out_size = nullptr);

  /**
   * @brief 先定位再读取数据。
   * @details 到达文件尾时允许返回 0 字节读取结果，由上层决定是否视为 EOF。
   */
  RC read_at(uint64_t offset, int size, char *data, int64_t *out_size = nullptr);

  /**
   * @brief 移动当前文件偏移。
   */
  RC seek(uint64_t offset);

private:
  string file_name_;        ///< 当前绑定的文件路径。
  int    file_desc_ = -1;   ///< 当前打开的文件描述符，`-1` 表示尚未打开。
};
