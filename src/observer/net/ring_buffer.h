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

#include <stdint.h>

#include "common/sys/rc.h"
#include "common/lang/vector.h"

/**
 * @file ring_buffer.h
 * @brief 面向网络发送路径的小型环形缓冲区。
 */

/**
 * @brief 环形缓存，当前用于通讯写入数据时的缓存
 * @ingroup Communicator
 * @details 只维护单生产者/单消费者式的顺序读写语义，不内置锁。读指针通过 `data_size_` 和
 * `write_pos_` 推导，避免额外维护两套游标。
 */
class RingBuffer
{
public:
  /**
   * @brief 使用默认缓存大小的构造函数，默认大小16K
   */
  RingBuffer();

  /**
   * @brief 指定初始化大小的构造函数。
   * @param size 缓冲区总容量，单位字节。
   */
  explicit RingBuffer(int32_t size);

  virtual ~RingBuffer();

  /**
   * @brief 从缓存中拷贝数据到外部缓冲区。
   * @param buf 目标缓冲区。
   * @param size 期望读取的数据大小。
   * @param[out] read_size 实际读取的数据大小。
   */
  RC read(char *buf, int32_t size, int32_t &read_size);

  /**
   * @brief 从缓存中读取数据，不会移动读指针
   * @details 读取数据时直接返回缓存中的指针，不会移动读指针。读取完成后执行forward函数移动读指针。
   * @param[out] buf 指向当前可连续读取片段的起始地址。
   * @param[out] read_size 当前连续片段长度。
   */
  RC buffer(const char *&buf, int32_t &read_size);

  /**
   * @brief 将读指针向前移动size个字节
   * @details 通常在buffer函数读取数据后，调用forward函数移动读指针
   * @param size 移动的字节数，必须大于 0 且不超过当前可读数据量。
   */
  RC forward(int32_t size);

  /**
   * @brief 将数据写入缓存
   * @param buf 待写入的数据。
   * @param size 待写入的数据大小。
   * @param[out] write_size 实际写入的数据大小。
   */
  RC write(const char *buf, int32_t size, int32_t &write_size);

  /**
   * @brief 缓存的总容量
   */
  int32_t capacity() const { return static_cast<int32_t>(buffer_.size()); }

  /**
   * @brief 缓存中剩余的可写入数据的空间
   */
  int32_t remain() const { return capacity() - size(); }

  /**
   * @brief 缓存中已经写入数据的空间大小
   */
  int32_t size() const { return data_size_; }

private:
  /**
   * @brief 计算当前读指针位置。
   * @details 由于仅显式维护写指针和有效数据长度，读指针可通过“写指针减去有效数据量”反推。
   */
  int32_t read_pos() const { return (write_pos_ - this->size() + capacity()) % capacity(); }

private:
  vector<char> buffer_;         ///< 环形存储的底层内存。
  int32_t      data_size_ = 0;  ///< 当前有效数据量。
  int32_t      write_pos_ = 0;  ///< 下次写入的起始位置，范围始终在 `[0, capacity)`。
};
