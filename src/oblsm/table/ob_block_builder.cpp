/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "oblsm/table/ob_block_builder.h"
#include "oblsm/util/ob_coding.h"
#include "common/log/log.h"

namespace oceanbase {

void ObBlockBuilder::reset()
{
  // 一个 block 构造完成后可原地复用 builder。
  offsets_.clear();
  data_.clear();
}

RC ObBlockBuilder::add(const string_view &key, const string_view &value)
{
  RC rc = RC::SUCCESS;
  if (appro_size() + key.size() + value.size() + 2 * sizeof(uint32_t) > BLOCK_SIZE) {
    // block 装不下新的 kv 了。若当前 block 为空，说明单条 kv 自身就过大。
    if (offsets_.empty()) {
      LOG_ERROR("block is empty, but kv pair is too large, key size: %lu, value size: %lu", key.size(), value.size());
      return RC::UNIMPLEMENTED;
    }
    LOG_TRACE("block is full, can't add more kv pair");
    rc = RC::FULL;
  } else {
    // 记录当前 entry 起始偏移，然后把 key/value 追加到数据区。
    offsets_.push_back(data_.size());
    put_numeric<uint32_t>(&data_, key.size());
    data_.append(key.data(), key.size());
    put_numeric<uint32_t>(&data_, value.size());
    data_.append(value.data(), value.size());
  }
  return rc;
}

string ObBlockBuilder::last_key() const
{
  // 末条记录总在 offsets_ 的最后一个位置。
  string_view last_kv(data_.data() + offsets_.back(), data_.size() - offsets_.back());
  uint32_t    key_length = get_numeric<uint32_t>(last_kv.data());
  return string(last_kv.data() + sizeof(uint32_t), key_length);
}

string_view ObBlockBuilder::finish()
{
  // block 末尾追加：
  // 1. entry 数量；
  // 2. 每条 entry 的 offset；
  // 3. 数据区起始位置。
  // 解码时就可以从尾部反向恢复整个 block 结构。
  uint32_t data_size = data_.size();
  put_numeric<uint32_t>(&data_, offsets_.size());
  for (size_t i = 0; i < offsets_.size(); i++) {
    put_numeric<uint32_t>(&data_, offsets_[i]);
  }
  put_numeric<uint32_t>(&data_, data_size);
  return string_view(data_.data(), data_.size());
}

}  // namespace oceanbase
