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

#include <thread>

#include "common/thread/thread_pool_executor.h"
#include "common/log/log.h"
#include "common/queue/simple_queue.h"
#include "common/thread/thread_util.h"

using namespace std;

namespace common {

int ThreadPoolExecutor::init(const char *name, int core_pool_size, int max_pool_size, long keep_alive_time_ms)
{
  // 默认使用一个简单的无界队列来承接任务。
  unique_ptr<Queue<unique_ptr<Runnable>>> queue_ptr(new (nothrow) SimpleQueue<unique_ptr<Runnable>>());
  return init(name, core_pool_size, max_pool_size, keep_alive_time_ms, std::move(queue_ptr));
}

int ThreadPoolExecutor::init(const char *name, int core_pool_size, int max_pool_size, long keep_alive_time_ms,
    unique_ptr<Queue<unique_ptr<Runnable>>> &&work_queue)
{
  // 线程池只允许从 NEW 状态初始化一次，避免重复 init 破坏内部状态。
  if (state_ != State::NEW) {
    LOG_ERROR("invalid state. state=%d", state_);
    return -1;
  }

  // 校验核心参数，确保线程数量关系合法。
  if (core_pool_size < 0 || max_pool_size <= 0 || core_pool_size > max_pool_size) {
    LOG_ERROR("invalid argument. core_pool_size=%d, max_pool_size=%d", core_pool_size, max_pool_size);
    return -1;
  }

  // 记录线程池名字，后续日志和线程名都会使用它。
  if (name != nullptr) {
    pool_name_ = name;
  }

  // 保存线程池配置和任务队列。
  core_pool_size_     = core_pool_size;
  max_pool_size_      = max_pool_size;
  keep_alive_time_ms_ = chrono::milliseconds(keep_alive_time_ms);
  work_queue_         = std::move(work_queue);

  // 预热所有核心线程。核心线程会一直驻留，不依赖任务触发创建。
  while (static_cast<int>(threads_.size()) < core_pool_size_) {
    if (create_thread(true /*core_thread*/) != 0) {
      LOG_ERROR("create thread failed");
      return -1;
    }
  }

  // 到这里说明基础资源都准备好了，线程池进入可运行状态。
  state_ = State::RUNNING;
  return 0;
}

ThreadPoolExecutor::~ThreadPoolExecutor()
{
  // 析构时做一次兜底回收，避免调用方忘记手动关闭线程池。
  if (state_ != State::TERMINATED) {
    shutdown();
    await_termination();
  }
}

int ThreadPoolExecutor::shutdown()
{
  // 只有运行中的线程池才需要切换到终止态。
  if (state_ != State::RUNNING) {
    return 0;
  }

  // shutdown 只负责“宣告停止接单”，不阻塞等待线程退出。
  state_ = State::TERMINATING;
  return 0;
}

int ThreadPoolExecutor::execute(const function<void()> &callable)
{
  // 普通函数对象统一适配成 Runnable，复用同一套提交逻辑。
  unique_ptr<Runnable> task_ptr(new RunnableAdaptor(callable));
  return this->execute(std::move(task_ptr));
}

int ThreadPoolExecutor::execute(unique_ptr<Runnable> &&task)
{
  // 线程池一旦不在 RUNNING 状态，就不再接收新任务。
  if (state_ != State::RUNNING) {
    LOG_WARN("[%s] cannot submit task. state=%d", pool_name_.c_str(), state_);
    return -1;
  }

  // 先把任务压入队列，再根据当前积压情况判断是否需要扩容。
  int ret       = work_queue_->push(std::move(task));
  int task_size = work_queue_->size();

  // 如果排队任务已经多于当前可用的空闲工作能力，就尝试多开一个线程。
  if (task_size > pool_size() - active_count()) {
    extend_thread();
  }
  return ret;
}

int ThreadPoolExecutor::await_termination()
{
  // 只有在线程池已经收到 shutdown 信号后，等待退出才有意义。
  if (state_ != State::TERMINATING) {
    return -1;
  }

  // 工作线程会在退出前把自己从 threads_ 中删除，这里轮询直到线程列表清空。
  while (threads_.size() > 0) {
    this_thread::sleep_for(200ms);
  }
  return 0;
}

void ThreadPoolExecutor::thread_func()
{
  // 工作线程启动后的第一件事是记录日志，便于排查线程生命周期。
  LOG_INFO("[%s] thread started", pool_name_.c_str());

  // 尝试把 OS 线程名设置成线程池名，方便在系统工具中定位。
  int ret = thread_set_name(pool_name_.c_str());
  if (ret != 0) {
    LOG_WARN("[%s] set thread name failed", pool_name_.c_str());
  }

  // 在线程表里找到当前线程对应的元数据，后续循环会持续更新这份状态。
  lock_.lock();
  auto iter = threads_.find(this_thread::get_id());
  if (iter == threads_.end()) {
    LOG_WARN("[%s] cannot find thread state of %lx", pool_name_.c_str(), this_thread::get_id());
    return;
  }
  ThreadData &thread_data = iter->second;
  lock_.unlock();

  using Clock = chrono::steady_clock;

  // 非核心线程从启动开始就有一个“空闲退出截止时间”；
  // 核心线程不依赖这个时间，下面的循环条件会让它一直运行。
  chrono::time_point<Clock> idle_deadline = Clock::now();
  if (!thread_data.core_thread && keep_alive_time_ms_.count() > 0) {
    idle_deadline += keep_alive_time_ms_;
  }

  // 这里采用非常直接的存活策略：
  // 1. 核心线程一直循环直到线程池关闭且队列清空；
  // 2. 非核心线程只要在 keep_alive 时间内没有闲死，就继续尝试取任务。
  //
  // 这种实现简单，但伸缩策略并不精细。
  // 例如任务突发结束后，线程回收并不是立即发生，而是依赖空闲超时。
  while (thread_data.core_thread || Clock::now() < idle_deadline) {
    unique_ptr<Runnable> task;

    // 尝试从任务队列取一个任务。
    int ret = work_queue_->pop(task);
    if (0 == ret && task) {
      // 拿到任务后，先把线程状态切到忙碌，并更新活跃线程计数。
      thread_data.idle = false;
      ++active_count_;

      // 执行任务本体。线程池不关心任务内部细节，只负责调度。
      task->run();

      // 任务执行完成后恢复线程为空闲，并更新统计信息。
      --active_count_;
      thread_data.idle = true;
      ++task_count_;

      // 非核心线程每处理完一个任务，都会刷新自己的空闲超时截止时间。
      // 这意味着只要它持续有活干，就不会因为启动时的老 deadline 过期而退出。
      if (keep_alive_time_ms_.count() > 0) {
        idle_deadline = Clock::now() + keep_alive_time_ms_;
      }
    } else {
      // 没有任务可取时，短暂休眠一下，避免忙等占满 CPU。
      this_thread::sleep_for(10ms);
    }

    // 一旦线程池进入关闭态，并且队列已经清空，就允许线程退出主循环。
    if (state_ != State::RUNNING && work_queue_->size() == 0) {
      break;
    }
  }

  // 走到这里说明线程准备退出，先把本线程的元数据标记成 terminated。
  thread_data.terminated = true;

  // 线程对象由线程自己负责 detach 和 delete。
  // detach 之后底层执行流与 std::thread 对象解绑，再释放包装对象本身。
  thread_data.thread_ptr->detach();
  delete thread_data.thread_ptr;
  thread_data.thread_ptr = nullptr;

  // 把自己从线程表中删除，await_termination 会靠这个状态感知线程是否退完。
  lock_.lock();
  threads_.erase(this_thread::get_id());
  lock_.unlock();

  // 线程生命周期结束。
  LOG_INFO("[%s] thread exit", pool_name_.c_str());
}

int ThreadPoolExecutor::create_thread(bool core_thread)
{
  // 对线程表的写操作统一放在锁内，避免并发创建破坏内部结构。
  lock_guard guard(lock_);
  return create_thread_locked(core_thread);
}

int ThreadPoolExecutor::create_thread_locked(bool core_thread)
{
  // 真正创建底层工作线程，并把入口函数绑定到当前线程池实例。
  thread *thread_ptr = new (nothrow) thread(&ThreadPoolExecutor::thread_func, this);
  if (thread_ptr == nullptr) {
    LOG_ERROR("create thread failed");
    return -1;
  }

  // 在线程表中登记这个新线程，方便后续查询状态和统计信息。
  ThreadData thread_data;
  thread_data.core_thread        = core_thread;
  thread_data.idle               = true;
  thread_data.terminated         = false;
  thread_data.thread_ptr         = thread_ptr;
  threads_[thread_ptr->get_id()] = thread_data;

  // 记录线程池历史上达到过的最大线程数。
  if (static_cast<int>(threads_.size()) > largest_pool_size_) {
    largest_pool_size_ = static_cast<int>(threads_.size());
  }
  return 0;
}

int ThreadPoolExecutor::extend_thread()
{
  lock_guard guard(lock_);

  // 已经达到线程上限时，直接放弃扩容。
  if (pool_size() >= max_pool_size_) {
    return 0;
  }

  // 如果当前空闲线程已经足够覆盖排队任务，就没必要继续加线程。
  if (work_queue_->size() <= pool_size() - active_count()) {
    return 0;
  }

  // 只有在“没到上限”且“空闲线程不够”时，才创建一个新的非核心线程。
  return create_thread_locked(false /*core_thread*/);
}

}  // end namespace common
