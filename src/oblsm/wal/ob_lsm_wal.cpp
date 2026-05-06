/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
   miniob is licensed under Mulan PSL v2.
   You can use this software according to the terms and conditions of the Mulan PSL v2.
   You may obtain a copy of Mulan PSL v2 at:
            http://license.coscl.org.cn/MulanPSL2
   THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
   EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
   MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
   See the Mulan PSL v2 for more details. */

#include "oblsm/wal/ob_lsm_wal.h"
#include "common/log/log.h"
#include "oblsm/util/ob_file_reader.h"

namespace oceanbase {
RC WAL::recover(const std::string &wal_file, std::vector<WalRecord> &wal_records)
{
  // TODO: 这里最终应当顺序扫描 wal_file，并把每条日志反序列化为 WalRecord。
  // 恢复顺序必须与原始写入顺序一致，因为 seq 的推进依赖它。
  // 在整条恢复链路里，Manifest 只负责告诉系统“该看哪个 WAL”，
  // 具体把 WAL 里的哪几条写回 memtable，由这里完成。
  return RC::UNIMPLEMENTED;
}

RC WAL::put(uint64_t seq, string_view key, string_view val)
{
  // TODO: 这里最终应当把 `(seq, key, val)` 以 WAL 记录格式追加到文件末尾。
  // put 成功后，调用方才会把这条记录写入 MemTable。
  // 这体现了 WAL 的核心职责：用顺序追加换取崩溃恢复能力，而不是参与读路径。
  return RC::UNIMPLEMENTED;
}
}  // namespace oceanbase
