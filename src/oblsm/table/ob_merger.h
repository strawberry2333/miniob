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

#include "common/lang/vector.h"
#include "common/lang/memory.h"

namespace oceanbase {

class ObComparator;
class ObLsmIterator;

/**
 * @brief 构造多路归并迭代器。
 *
 * 它会把多个已经各自有序的子迭代器合并成一条全局有序流。
 * 典型输入包括：
 * - 当前 MemTable；
 * - Immutable MemTable；
 * - 多个 SSTable。
 *
 * 这个归并器不负责“版本可见性”和“去重”，只负责维护全局顺序。
 * 更上层的 `ObUserIterator` 再在这个基础上做语义过滤。
 *
 * 因此它的职责边界很明确：
 * - 它不知道 value 的含义；
 * - 它不判断删除标记；
 * - 它也不合并相同 user key 的多个版本；
 * 它只是把多个“各自已经按 internal key 有序”的输入流拼成一个更大的有序流。
 */
ObLsmIterator *new_merging_iterator(const ObComparator *comparator, vector<unique_ptr<ObLsmIterator>> &&children);

}  // namespace oceanbase
