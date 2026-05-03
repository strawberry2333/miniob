/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

// mt_visible 保证这些符号在动态库中对外可见，
// 外部代码（如测试框架）可通过 dlsym 按名称查找并调用这些查询接口。
#define mt_visible __attribute__((visibility("default")))

namespace memtracer {

// 返回当前已分配的内存总量（字节），包含代码段基线和所有 malloc/mmap 分配
mt_visible size_t allocated_memory();

// 返回元数据内存占用（字节）：每次 malloc 在用户数据前存一个 size_t，
// 未释放的分配数乘以 sizeof(size_t) 即为当前元数据开销
mt_visible size_t meta_memory();

// 返回通过环境变量 MT_MEMORY_LIMIT 设置的内存上限（字节），
// 未设置时为 UINT64_MAX（即不限制）
mt_visible size_t memory_limit();

}  // namespace memtracer