/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "oblsm/util/ob_comparator.h"
#include "oblsm/ob_lsm_define.h"
#include "oblsm/util/ob_coding.h"

namespace oceanbase {

// 用户键没有额外元信息，直接使用 string_view 的字典序比较。
int ObDefaultComparator::compare(const string_view &a, const string_view &b) const { return a.compare(b); }

int ObInternalKeyComparator::compare(const string_view &a, const string_view &b) const
{
  // internal key 先按 user key 分组，确保同一主键的不同版本会落在相邻位置。
  const string_view akey = extract_user_key(a);
  const string_view bkey = extract_user_key(b);
  int               r    = default_comparator_.compare(akey, bkey);
  if (r == 0) {
    // user key 相同时，再读取尾部 seq。
    // 这里使用“新版本排前面”的倒序规则，便于上层只取第一个命中的版本。
    uint64_t aseq = get_numeric<uint64_t>(akey.data() + a.size() - SEQ_SIZE);
    uint64_t bseq = get_numeric<uint64_t>(bkey.data() + b.size() - SEQ_SIZE);
    if (aseq > bseq) {
      r = -1;
    } else if (aseq < bseq) {
      r = +1;
    }
  }
  return r;
}

}  // namespace oceanbase
