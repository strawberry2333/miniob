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

#ifdef __MUSL__
#include <errno.h>
#else
#include <sys/errno.h>
#endif
#include <unistd.h>

#include "net/buffered_writer.h"

/**
 * @file buffered_writer.cpp
 * @brief BufferedWriter 的顺序写入实现。
 */

BufferedWriter::BufferedWriter(int fd) : fd_(fd), buffer_() {}

BufferedWriter::BufferedWriter(int fd, int32_t size) : fd_(fd), buffer_(size) {}

BufferedWriter::~BufferedWriter() { close(); }

RC BufferedWriter::close()
{
  if (fd_ < 0) {
    return RC::SUCCESS;
  }

  // close 前先把用户态缓冲区里的数据尽量刷干净，避免响应截断。
  RC rc = flush();
  if (OB_FAIL(rc)) {
    return rc;
  }

  // 这里只标记写入器不可用，不主动关闭底层连接 fd。
  fd_ = -1;

  return RC::SUCCESS;
}

RC BufferedWriter::write(const char *data, int32_t size, int32_t &write_size)
{
  if (fd_ < 0) {
    return RC::INVALID_ARGUMENT;
  }

  if (buffer_.remain() == 0) {
    // 缓冲区已满时先尝试刷出一部分，为本次写入腾空间。
    RC rc = flush_internal(size);
    if (OB_FAIL(rc)) {
      return rc;
    }
  }

  return buffer_.write(data, size, write_size);
}

RC BufferedWriter::writen(const char *data, int32_t size)
{
  if (fd_ < 0) {
    return RC::INVALID_ARGUMENT;
  }

  int32_t write_size = 0;
  while (write_size < size) {
    int32_t tmp_write_size = 0;

    // 循环写到 BufferedWriter 全量接收为止；真正落到 socket 可能发生在之后的 flush。
    RC rc = write(data + write_size, size - write_size, tmp_write_size);
    if (OB_FAIL(rc)) {
      return rc;
    }

    write_size += tmp_write_size;
  }

  return RC::SUCCESS;
}

RC BufferedWriter::flush()
{
  if (fd_ < 0) {
    return RC::INVALID_ARGUMENT;
  }

  RC rc = RC::SUCCESS;
  while (OB_SUCC(rc) && buffer_.size() > 0) {
    // 每次按当前可读数据量推进，直到环形缓冲区被完全清空。
    rc = flush_internal(buffer_.size());
  }
  return rc;
}

RC BufferedWriter::flush_internal(int32_t size)
{
  if (fd_ < 0) {
    return RC::INVALID_ARGUMENT;
  }

  RC rc = RC::SUCCESS;

  int32_t write_size = 0;
  while (OB_SUCC(rc) && buffer_.size() > 0 && size > write_size) {
    const char *buf       = nullptr;
    int32_t     read_size = 0;
    rc                    = buffer_.buffer(buf, read_size);  // 取当前连续可写出的一段。
    if (OB_FAIL(rc)) {
      return rc;
    }

    ssize_t tmp_write_size = 0;
    while (tmp_write_size == 0) {
      // 非阻塞 fd 上可能遇到 EINTR/EAGAIN，这两种情况重试即可。
      tmp_write_size = ::write(fd_, buf, read_size);
      if (tmp_write_size < 0) {
        if (errno == EAGAIN || errno == EINTR) {
          tmp_write_size = 0;
          continue;
        } else {
          return RC::IOERR_WRITE;
        }
      }
    }

    write_size += tmp_write_size;
    buffer_.forward(tmp_write_size);  // 只在真正写成功后推进读指针。
  }

  return rc;
}
