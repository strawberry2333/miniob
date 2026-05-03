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
// Created by Longda on 2021/4/14.
//

#pragma once

/**
 * @brief observer 读取 ini 配置时使用的键名常量。
 * @details 这些宏只负责描述配置项的字符串名字和默认值，不包含解析逻辑。
 */

/// @brief 网络监听地址在 ini 文件中的键名。
#define CLIENT_ADDRESS "CLIENT_ADDRESS"
/// @brief 最大连接数配置项名。
#define MAX_CONNECTION_NUM "MAX_CONNECTION_NUM"
/// @brief 最大连接数默认值。
#define MAX_CONNECTION_NUM_DEFAULT 8192
/// @brief 网络监听端口配置项名。
#define PORT "PORT"
/// @brief 默认监听端口。
#define PORT_DEFAULT 6789

/// @brief socket 收发缓冲区大小。
#define SOCKET_BUFFER_SIZE 8192

/// @brief SessionStage 在配置中的逻辑名字。
#define SESSION_STAGE_NAME "SessionStage"
