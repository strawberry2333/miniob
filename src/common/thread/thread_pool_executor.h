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
// Created by Wangyunlai on 2023/01/11.
//

#pragma once

#include <stdint.h>

#include "common/queue/queue.h"
#include "common/thread/runnable.h"
#include "common/lang/mutex.h"
#include "common/lang/atomic.h"
#include "common/lang/memory.h"
#include "common/lang/map.h"
#include "common/lang/chrono.h"
#include "common/lang/thread.h"

namespace common {

/**
 * @brief 模拟 Java ThreadPoolExecutor 的一个简化线程池
 * @defgroup ThreadPool
 * @details 线程池由两部分组成：
 * 1. 一个任务队列，用来缓存尚未执行的任务；
 * 2. 一组工作线程，用来持续从队列中取任务并执行。
 *
 * 这里的接口设计参考了 Java 的 ThreadPoolExecutor，但实现做了明显简化：
 * 1. 只保留最核心的提交、扩容、关闭、等待退出能力；
 * 2. 不支持拒绝策略、自定义线程工厂、future 返回值等高级特性；
 * 3. 任务调度策略也比较直接，优先保证实现易懂。
 *
 * 线程池支持自动伸缩：
 * 1. 核心线程会在 init 阶段预先创建，并长期存活；
 * 2. 非核心线程只在任务堆积时按需创建；
 * 3. 非核心线程在空闲超过 keep_alive_time_ms 后会自行退出。
 *
 * 当前扩容策略比较朴素：
 * 当队列里的待执行任务数大于“当前线程数 - 活跃线程数”时，说明空闲线程可能不够用，
 * 线程池会尝试再创建一个非核心线程。
 *
 * TODO 任务execute接口，增加一个future返回值，可以获取任务的执行结果
 */
class ThreadPoolExecutor
{
public:
  ThreadPoolExecutor() = default;
  /**
   * @brief 析构时兜底关闭线程池
   * @details 如果调用方忘记显式调用 shutdown/await_termination，
   * 析构函数会主动触发一次关闭流程，尽量避免后台线程泄漏。
   */
  virtual ~ThreadPoolExecutor();

  /**
   * @brief 初始化线程池
   *
   * @param name 线程池名称
   * @param core_size 核心线程个数。核心线程不会退出
   * @param max_size  线程池最大线程个数
   * @param keep_alive_time_ms 非核心线程空闲多久后退出
   */
  int init(const char *name, int core_size, int max_size, long keep_alive_time_ms);

  /**
   * @brief 初始化线程池
   *
   * @param name 线程池名称
   * @param core_size 核心线程个数。核心线程不会退出
   * @param max_size  线程池最大线程个数
   * @param keep_alive_time_ms 非核心线程空闲多久后退出
   * @param work_queue 任务队列
   */
  int init(const char *name, int core_pool_size, int max_pool_size, long keep_alive_time_ms,
      unique_ptr<Queue<unique_ptr<Runnable>>> &&work_queue);

  /**
   * @brief 提交一个 Runnable 任务，不一定会立刻执行
   *
   * @param task 待执行任务，所有权会转移到线程池
   * @return int 成功入队返回 0，失败返回非 0
   */
  int execute(unique_ptr<Runnable> &&task);

  /**
   * @brief 提交一个普通可调用对象，不一定会立刻执行
   *
   * @param callable 会被包装成 RunnableAdaptor 后交给线程池
   * @return int 成功入队返回 0，失败返回非 0
   */
  int execute(const function<void()> &callable);

  /**
   * @brief 发起关闭流程
   * @details 关闭后不再接收新任务，但已经入队的任务仍会继续执行，
   * 工作线程会在队列清空后自行退出。
   */
  int shutdown();
  /**
   * @brief 等待线程池处理完所有任务并退出
   * @details 该函数依赖工作线程在退出时主动从 threads_ 中移除自己，
   * 因此它本质上是在轮询线程池是否已经清空。
   */
  int await_termination();

public:
  /**
   * @brief 当前活跃线程的个数，就是正在处理任务的线程个数
   */
  int active_count() const { return active_count_.load(); }
  /**
   * @brief 核心线程个数
   */
  int core_pool_size() const { return core_pool_size_; }
  /**
   * @brief 线程池中线程个数
   */
  int pool_size() const { return static_cast<int>(threads_.size()); }
  /**
   * @brief 曾经达到过的最大线程个数
   */
  int largest_pool_size() const { return largest_pool_size_; }
  /**
   * @brief 处理过的任务个数
   */
  int64_t task_count() const { return task_count_.load(); }

  /**
   * @brief 任务队列中的任务个数
   */
  int64_t queue_size() const { return static_cast<int64_t>(work_queue_->size()); }

private:
  /**
   * @brief 创建一个线程
   *
   * @param core_thread 是否创建为核心线程
   */
  int create_thread(bool core_thread);
  /**
   * @brief 在已持锁的前提下创建一个线程
   *
   * @param core_thread 是否创建为核心线程
   */
  int create_thread_locked(bool core_thread);
  /**
   * @brief 根据当前排队任务和线程空闲情况决定是否扩容
   */
  int extend_thread();

private:
  /**
   * @brief 工作线程主循环
   * @details 线程启动后会不断从任务队列拉取任务执行。
   * 核心线程会一直存活到线程池关闭；
   * 非核心线程会在空闲超时后退出。
   */
  void thread_func();

private:
  /**
   * @brief 线程池生命周期状态
   */
  enum class State
  {
    NEW,          //! 已构造但尚未 init
    RUNNING,      //! 正常运行，可接收新任务
    TERMINATING,  //! 已发起关闭，不再接收新任务，等待队列清空
    TERMINATED    //! 全部工作线程已退出
  };

  struct ThreadData
  {
    bool    core_thread = false;    /// 是否为核心线程。核心线程不会因空闲超时退出
    bool    idle        = false;    /// 线程当前是否空闲。仅用于粗粒度状态描述
    bool    terminated  = false;    /// 线程是否已走到退出流程
    thread *thread_ptr  = nullptr;  /// 对应的 std::thread 对象，在线程退出时负责回收
  };

private:
  State state_ = State::NEW;  /// 线程池当前状态

  int                  core_pool_size_ = 0;  /// 期望长期保留的核心线程数
  int                  max_pool_size_  = 0;  /// 线程池允许扩到的最大线程数
  chrono::milliseconds keep_alive_time_ms_;  /// 非核心线程空闲多久后可以退出

  unique_ptr<Queue<unique_ptr<Runnable>>> work_queue_;  /// 待执行任务队列

  mutable mutex               lock_;     /// 保护 threads_ 等共享状态
  map<thread::id, ThreadData> threads_;  /// 线程 id 到线程元数据的映射

  int             largest_pool_size_ = 0;  /// 历史上达到过的最大线程数
  atomic<int64_t> task_count_        = 0;  /// 已执行完成的任务总数
  atomic<int>     active_count_      = 0;  /// 当前正在执行任务的线程数
  string          pool_name_;              /// 线程池名称，用于日志和线程命名
};

}  // namespace common
