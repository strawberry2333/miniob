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
// Created by Longda on 2021/5/3.
//

#pragma once

#include "common/conf/ini.h"
#include "common/os/process_param.h"

/**
 * @brief observer 进程初始化入口。
 * @param processParam 启动参数集合，包含配置文件、协议、线程模型等运行期开关。
 * @return 0 表示初始化成功，非 0 表示初始化失败。
 */
int  init(common::ProcessParam *processParam);

/**
 * @brief observer 进程收尾入口。
 * @details 负责释放 init() 创建的全局对象、配置和日志资源。
 */
void cleanup();
