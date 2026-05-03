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
// Created by Wangyunlai on 2023/5/29.
//

#include "common/global_context.h"

/**
 * @brief GlobalContext 的实现文件。
 * @details 通过一个进程级静态对象统一承载 observer 运行期间的共享上下文。
 */

static GlobalContext global_context;

/// @brief 返回唯一的全局上下文实例。
GlobalContext &GlobalContext::instance() { return global_context; }
