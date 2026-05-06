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

#include "common/lang/string.h"
#include "common/lang/string_view.h"
#include "common/lang/vector.h"
#include "common/sys/rc.h"

namespace oceanbase {

/**
 * @brief 负责把一批有序 key/value 增量构造成一个 block。
 *
 * 典型使用方式：
 * 1. 按 key 顺序不断调用 `add()`；
 * 2. 达到块大小上限或输入结束时调用 `finish()`；
 * 3. 把返回的字节流写入 SSTable。
 */
class ObBlockBuilder
{

public:
  // 追加一条 entry；若 block 已满则返回 `RC::FULL`。
  RC add(const string_view &key, const string_view &value);

  // 完成 block 编码，返回可直接写盘的字节视图。
  string_view finish();

  // 丢弃当前 block 状态，准备构造下一个 block。
  void reset();

  // 返回当前 block 中最后一条 entry 的 key，供生成 BlockMeta 使用。
  string last_key() const;

  // 近似大小 = 数据区 + offset 数组，作为切块依据。
  uint32_t appro_size() { return data_.size() + offsets_.size() * sizeof(uint32_t); }

private:
  static const uint32_t BLOCK_SIZE = 4 * 1024;  // 单块目标大小
  // 每条 entry 在 data_ 中的起始偏移。
  vector<uint32_t> offsets_;
  // 顺序拼接的 entry 数据区。
  string data_;
};

}  // namespace oceanbase
