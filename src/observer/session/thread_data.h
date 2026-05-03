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

#pragma once

class Trx;
class Session;

/**
 * @brief 线程局部运行时上下文。
 * @details 这个结构比 Session 更轻量，主要用于让底层代码通过 thread_local 入口取到当前线程关联的 Session/Trx。
 */
class ThreadData
{
public:
  /// @brief 返回当前线程绑定的 ThreadData；未初始化时返回空。
  static ThreadData *current() { return thread_data_; }
  /// @brief 把一个 ThreadData 实例绑定到当前线程。
  static void        setup(ThreadData *thread) { thread_data_ = thread; }

public:
  ThreadData()  = default;
  ~ThreadData() = default;

  /// @brief 返回当前线程关联的 Session。
  Session *session() const { return session_; }
  /// @brief 返回当前线程可见的事务对象；若 session 为空则返回空。
  Trx     *trx() const;

  /// @brief 更新当前线程关联的 Session。
  void set_session(Session *session) { session_ = session; }

private:
  static thread_local ThreadData *thread_data_;

private:
  Session *session_ = nullptr;  ///< 当前线程正在服务的会话，不拥有其生命周期。
};
