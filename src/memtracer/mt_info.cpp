/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */
#include "memtracer.h"
#include "mt_info.h"

namespace memtracer {

// 以下三个函数是对 MemTracer 单例的薄封装，对外暴露只读查询接口。
// 独立成文件是为了让外部模块（如基准测试、监控脚本）通过 dlsym 动态获取内存数据，
// 而不需要依赖完整的 MemTracer 头文件。

mt_visible size_t allocated_memory() { return MT.allocated_memory(); }

mt_visible size_t meta_memory() { return MT.meta_memory(); }

mt_visible size_t memory_limit() { return MT.memory_limit(); }

}  // namespace memtracer
