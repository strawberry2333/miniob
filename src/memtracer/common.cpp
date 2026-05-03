/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include <cstdarg>
#include <sstream>
#include "memtracer/common.h"

namespace memtracer {

// 将格式化日志输出到 stderr。
// 不使用 printf 是因为 stdout 有缓冲，进程异常退出时可能丢失日志；
// stderr 默认无缓冲，崩溃时日志仍能完整输出。
void log_stderr(const char *format, ...)
{
  va_list vl;
  va_start(vl, format);
  vfprintf(stderr, format, vl);
  va_end(vl);
}

}  // namespace memtracer