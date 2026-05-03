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
// Created by Wangyunlai on 2023/06/16.
//

#pragma once

#include "net/ring_buffer.h"

/**
 * @file buffered_writer.h
 * @brief 带用户态缓存的顺序写入器。
 * @details 当前主要服务于网络返回路径，将多次小写合并到环形缓冲区中，必要时再批量刷到
 * socket 或文件描述符，减少系统调用次数。
 */

/**
 * @brief 支持以缓存模式写入数据到文件/socket。
 * @details 缓存使用 RingBuffer 实现，当缓存满时会自动刷新缓存。该对象只负责缓存与刷盘/
 * 刷 socket，不拥有底层 fd 的关闭语义，因此 close 仅表示“停止使用并尝试刷出剩余数据”。
 */
class BufferedWriter
{
public:
  /**
   * @brief 使用默认缓存大小初始化写入器。
   * @param fd 已经打开且可写的文件描述符。
   */
  BufferedWriter(int fd);

  /**
   * @brief 使用指定缓存大小初始化写入器。
   * @param fd 已经打开且可写的文件描述符。
   * @param size 环形缓冲区容量，单位字节。
   */
  BufferedWriter(int fd, int32_t size);
  ~BufferedWriter();

  /**
   * @brief 关闭缓存视图并尽力刷出剩余数据。
   * @return RC::SUCCESS 表示缓存内容已经处理完成；失败时调用方需要自行决定是否关闭连接。
   */
  RC close();

  /**
   * @brief 写数据到缓存或底层 fd。
   * @details 这是“尽量写”语义：如果缓存已满，会先触发一次局部 flush，然后把当前还能容纳的
   * 数据拷入缓存；返回的 write_size 可能小于 size，调用方可继续重试。
   * @param data 要写入的数据，不能为空。
   * @param size 要写入的数据大小，单位字节。
   * @param write_size 实际进入缓存的数据大小。
   */
  RC write(const char *data, int32_t size, int32_t &write_size);

  /**
   * @brief 写数据到文件/socket，直到全部进入缓存或发生不可恢复错误。
   * @details 与 write 的区别在于该接口具备“全量写入”语义，会循环调用 write 直到 size
   * 字节全部被接收。
   * @param data 要写入的数据，不能为空。
   * @param size 要写入的数据大小，单位字节。
   */
  RC writen(const char *data, int32_t size);

  /**
   * @brief 刷新缓存。
   * @details 将缓存中的全部数据按顺序写入到底层 fd。
   */
  RC flush();

private:
  /**
   * @brief 刷新缓存中的一部分数据。
   * @details 调用方通常在缓存写满时希望先腾出至少 size 字节空间。由于底层 write 以当前可读
   * 连续片段为单位发送，实际刷新的字节数可能与 size 不完全一致。
   * @param size 希望至少刷出的字节数。
   */
  RC flush_internal(int32_t size);

private:
  int        fd_ = -1;  ///< 目标 fd；设为 -1 表示当前写入器不可再用。
  RingBuffer buffer_;   ///< 用户态发送缓冲区，保存尚未真正写到底层 fd 的数据。
};
