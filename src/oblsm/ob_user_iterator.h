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

namespace oceanbase {

class ObComparator;
class ObLsmIterator;

/**
 * @brief 基于内部迭代器构造“用户视角”的迭代器。
 *
 * 内部迭代器通常输出的是按 internal key 排序的数据流，其中包含：
 * - 同一个 user key 的多个历史版本；
 * - value 为空的删除标记；
 * - 来自 MemTable、Immutable MemTable、SSTable 的混合数据。
 *
 * UserIterator 的职责就是把这条内部流转换成上层真正想看到的结果：
 * - 只保留 `seq` 可见范围内的版本；
 * - 同一个 user key 只返回最新可见版本；
 * - 遇到 tombstone 时跳过该 key。
 *
 * 易错点：这里的 `seq` 是“最大可见版本号”，不是要精确命中的版本号。
 * 因此 seek 和 next 都不能只比较 user key，还必须结合 internal key 中的版本号做过滤。
 * 返回对象会接管传入 `iterator` 的生命周期，调用方无需再单独释放底层迭代器。
 */
ObLsmIterator *new_user_iterator(ObLsmIterator *iterator, uint64_t seq);

}  // namespace oceanbase
