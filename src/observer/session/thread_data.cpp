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
// Created by Wangyunlai on 2023/03/07.
//

#include "session/thread_data.h"
#include "session/session.h"

/**
 * @brief ThreadData 的实现文件。
 * @details 这里的逻辑刻意保持极简，只提供 thread_local 指针存取和事务透传。
 */

/// @brief 每个工作线程各自维护一份 ThreadData 指针。
thread_local ThreadData *ThreadData::thread_data_;

/**
 * @brief 返回当前线程关联会话上的事务对象。
 * @return 若 session_ 非空则返回 session_->current_trx()，否则返回空。
 */
Trx *ThreadData::trx() const { return (session_ == nullptr) ? nullptr : session_->current_trx(); }
