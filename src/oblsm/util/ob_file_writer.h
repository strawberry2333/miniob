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
#include "common/lang/string.h"
#include "common/lang/string_view.h"
#include "common/lang/memory.h"
#include "common/sys/rc.h"

namespace oceanbase {

/**
 * @class ObFileWriter
 * @brief 面向顺序写场景的文件输出封装。
 *
 * 这个类主要服务于 oblsm 的持久化写路径，例如：
 * - 生成新的 SSTable 文件；
 * - 追加写 Manifest；
 * - 顺序写入 WAL。
 *
 * 当前实现基于 `std::ofstream`，强调接口简单而不是极致控制力。
 * 它提供的能力只有：
 * - 打开文件；
 * - 追加一段原始字节；
 * - 将用户态缓冲区刷新到内核。
 *
 * @note
 * 1. `flush()` 只做流缓冲刷新，不等价于 `fsync`；
 * 2. 打开模式由 `append_` 决定，是“追加写”还是“截断重建”；
 * 3. `write()` 视 `string_view` 为原始二进制数据，不附加额外分隔符。
 */
class ObFileWriter
{
public:
  /**
   * @brief 构造一个文件写入器。
   *
   * 构造阶段只保存目标文件名与打开模式，不会立刻创建底层文件。
   *
   * @param filename 目标文件路径。
   * @param append 为 `true` 时以后追加模式打开，否则以截断模式重建文件。
   */
  ObFileWriter(const string &filename, bool append = false) : filename_(filename), append_(append) {}

  ~ObFileWriter();

  /**
   * @brief 打开文件以便后续写入。
   *
   * 若 `append_` 为真，则保留现有内容并从文件尾继续写；
   * 否则使用 `trunc` 清空旧内容，重新生成文件。
   *
   * @return 成功返回 `RC::SUCCESS`，失败返回错误码。
   */
  RC open_file();

  /**
   * @brief 关闭文件。
   *
   * 关闭前会先尝试 `flush()` 当前缓冲区，再释放流对象。
   */
  void close_file();

  /**
   * @brief 将一段原始字节写入文件。
   *
   * 这里不会对内容做编码或换行处理，调用方需要自行组织 WAL record、
   * block payload 或其他磁盘格式。
   *
   * @param data 待写入字节序列。
   * @return 成功返回 `RC::SUCCESS`，失败返回写错误码。
   */
  RC write(const string_view &data);

  /**
   * @brief 刷新 `ofstream` 的用户态缓冲。
   *
   * 该操作能推动数据从 C++ 流缓冲写入内核，但不保证已经落到稳定存储介质。
   * 如果上层要求 WAL 级别的强持久化，后续需要额外的 `fsync/fdatasync` 语义。
   *
   * @return 成功返回 `RC::SUCCESS`，失败返回同步错误码。
   */
  RC flush();

  /**
   * @brief 判断底层流是否已经打开。
   */
  bool is_open() const { return file_.is_open(); }

  /**
   * @brief 返回目标文件路径。
   */
  string file_name() const { return filename_; }

  /**
   * @brief 创建并立即打开一个写入器。
   *
   * 适合调用方在只关心“能否立刻写入”的场景下使用。
   *
   * @param filename 文件路径。
   * @param append 是否使用追加模式。
   * @return 成功返回已打开对象，失败返回 `nullptr`。
   */
  static unique_ptr<ObFileWriter> create_file_writer(const string &filename, bool append);

private:
  /**
   * @brief 目标文件路径。
   */
  string filename_;

  /**
   * @brief 打开模式选择。
   *
   * `true` 表示保留已有内容并从尾部追加；
   * `false` 表示打开时截断旧文件。
   */
  bool append_;

  /**
   * @brief 底层输出流。
   */
  ofstream file_;
};
}  // namespace oceanbase
