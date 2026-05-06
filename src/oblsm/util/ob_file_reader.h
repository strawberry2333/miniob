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

#include "common/lang/fstream.h"
#include "common/lang/memory.h"
#include "common/lang/sstream.h"
#include "common/lang/string.h"
#include "common/sys/rc.h"
#include "common/lang/mutex.h"

namespace oceanbase {

/**
 * @class ObFileReader
 * @brief 基于文件描述符的只读访问封装。
 *
 * 这个类服务于 oblsm 的磁盘读路径，例如：
 * - 按偏移读取 SSTable 中的 block；
 * - 读取元数据段或 footer；
 * - 在 WAL / Manifest 恢复时顺序抓取原始字节。
 *
 * 设计上它刻意保持简单：
 * - `open_file()` 负责建立只读 fd；
 * - `read_pos()` 负责按给定偏移做定长读取；
 * - `file_size()` 负责从文件系统查询当前文件大小。
 *
 * @note
 * 1. 当前实现不做缓存，也不做重试；
 * 2. 调用方需要确保文件已经成功打开，且读取范围合法；
 * 3. `read_pos()` 要求一次读满指定字节数，否则返回空串表示失败。
 */
class ObFileReader
{
public:
  /**
   * @brief 构造一个文件读取器。
   *
   * 构造函数只记录文件名，不会立即打开文件；
   * 这样便于上层先创建对象，再在合适的时机建立底层 fd。
   *
   * @param filename 目标文件路径。
   */
  ObFileReader(const string &filename) : filename_(filename) {}

  ~ObFileReader();

  /**
   * @brief 以只读方式打开文件。
   *
   * 内部使用 `::open(..., O_RDONLY)` 获取文件描述符。
   * 若打开失败，调用方无法继续使用 `read_pos()`。
   *
   * @return 成功返回 `RC::SUCCESS`，失败返回错误码。
   */
  RC open_file();

  /**
   * @brief 关闭当前 fd。
   *
   * 当前实现会直接对 `fd_` 调用 `::close()`，并依赖析构时自动收尾。
   * 由于函数本身不返回状态码，若底层关闭失败不会向上传递。
   */
  void close_file();

  /**
   * @brief 从指定偏移读取固定长度字节。
   *
   * 这里使用 `pread`，因此读取不会改变文件游标；
   * 这很适合按 block 偏移随机读取，也避免了多个读操作互相干扰文件位置。
   *
   * @param pos 起始偏移。
   * @param size 期望读取的字节数。
   * @return 成功时返回长度为 `size` 的字节串；若发生短读或错误则返回空串。
   */
  string read_pos(uint32_t pos, uint32_t size);

  /**
   * @brief 获取文件当前大小。
   *
   * 这里按文件名向文件系统查询，而不是依赖已打开 fd 的当前位置。
   *
   * @return 文件字节数。
   */
  uint32_t file_size();

  /**
   * @brief 创建并立即打开一个读取器。
   *
   * 这是便捷工厂：构造对象后立刻调用 `open_file()`。
   * 如果打开失败，则直接返回 `nullptr`，避免上层拿到半初始化对象。
   *
   * @param filename 文件路径。
   * @return 成功时返回已打开对象，失败返回 `nullptr`。
   */
  static unique_ptr<ObFileReader> create_file_reader(const string &filename);

private:
  /**
   * @brief 目标文件路径。
   */
  string filename_;

  /**
   * @brief 当前打开的文件描述符。
   *
   * 约定 `-1` 表示尚未打开或已经失效。
   */
  int fd_ = -1;
};

}  // namespace oceanbase
