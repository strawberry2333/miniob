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
// Created by Wangyunlai on 2024/01/10.
//

#include <poll.h>

#include "net/one_thread_per_connection_thread_handler.h"
#include "common/log/log.h"
#include "common/lang/thread.h"
#include "common/lang/mutex.h"
#include "common/lang/chrono.h"
#include "common/thread/thread_util.h"
#include "net/communicator.h"
#include "net/sql_task_handler.h"

using namespace common;

/**
 * @file one_thread_per_connection_thread_handler.cpp
 * @brief 每连接一线程模型的实现。
 */

/**
 * @brief 绑定单个连接生命周期的工作线程。
 * @details Worker 持有连接对象的非拥有指针，真正的释放由宿主 ThreadHandler 在
 * close_connection 中完成。
 */
class Worker
{
public:
  Worker(ThreadHandler &host, Communicator *communicator) 
    : host_(host), communicator_(communicator)
  {}
  ~Worker()
  {
    if (thread_ != nullptr) {
      // 防御式收尾：如果上层忘记显式停止，这里也尽量让后台线程退出。
      stop();
      join();
    }
  }

  RC start()
  {
    thread_ = new thread(ref(*this));
    return RC::SUCCESS;
  }

  RC stop()
  {
    running_ = false;
    return RC::SUCCESS;
  }

  RC join()
  {
    if (thread_) {
      if (thread_->get_id() == this_thread::get_id()) {
        thread_->detach(); // 当前 worker 线程在自清理路径上不能 join 自己，否则会死锁。
      } else {
        thread_->join();
      }
      delete thread_;
      thread_ = nullptr;
    }
    return RC::SUCCESS;
  }

  void operator()()
  {
    LOG_INFO("worker thread start. communicator = %p", communicator_);
    int ret = thread_set_name("SQLWorker");
    if (ret != 0) {
      LOG_WARN("failed to set thread name. ret = %d", ret);
    }

    struct pollfd poll_fd;
    poll_fd.fd = communicator_->fd();
    poll_fd.events = POLLIN;
    poll_fd.revents = 0;

    while (running_) {
      // 每个 worker 独占一个连接，所以可以直接同步 poll/read/execute/write。
      int ret = poll(&poll_fd, 1, 500);
      if (ret < 0) {
        LOG_WARN("poll error. fd = %d, ret = %d, error=%s", poll_fd.fd, ret, strerror(errno));
        break;
      } else if (0 == ret) {
        // LOG_TRACE("poll timeout. fd = %d", poll_fd.fd);
        continue;
      }

      if (poll_fd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
        LOG_WARN("poll error. fd = %d, revents = %d", poll_fd.fd, poll_fd.revents);
        break;
      }

      // handle_event 负责完整走完“收包 -> SQL 执行 -> 回包”闭环。
      RC rc = task_handler_.handle_event(communicator_);
      if (OB_FAIL(rc)) {
        LOG_ERROR("handle error. rc = %s", strrc(rc));
        break;
      }
    }

    LOG_INFO("worker thread stop. communicator = %p", communicator_);
    host_.close_connection(communicator_); /// 连接关闭后，当前 Worker/Communicator 都会走清理路径。
  }

private:
  ThreadHandler &host_;
  SqlTaskHandler task_handler_;
  Communicator *communicator_ = nullptr;
  thread *thread_ = nullptr;
  volatile bool running_ = true;
};

OneThreadPerConnectionThreadHandler::~OneThreadPerConnectionThreadHandler()
{
  stop();
  await_stop();
}

RC OneThreadPerConnectionThreadHandler::new_connection(Communicator *communicator)
{
  lock_guard guard(lock_);

  auto iter = thread_map_.find(communicator);
  if (iter != thread_map_.end()) {
    LOG_WARN("connection already exists. communicator = %p", communicator);
    return RC::FILE_EXIST;
  }

  Worker *worker = new Worker(*this, communicator);
  thread_map_[communicator] = worker;  // 先登记，再启动线程，保证异常时能按统一路径回收。
  return worker->start();
}

RC OneThreadPerConnectionThreadHandler::close_connection(Communicator *communicator)
{
  lock_.lock();
  auto iter = thread_map_.find(communicator);
  if (iter == thread_map_.end()) {
    LOG_WARN("connection not exists. communicator = %p", communicator);
    lock_.unlock();
    return RC::FILE_NOT_EXIST;
  }

  Worker *worker = iter->second;
  thread_map_.erase(iter);
  lock_.unlock();

  // 先把连接从映射中摘掉，再等待线程退出，避免其它路径继续命中这个连接。
  worker->stop();
  worker->join();
  delete worker;
  delete communicator;
  LOG_INFO("close connection. communicator = %p", communicator);
  return RC::SUCCESS;
}

RC OneThreadPerConnectionThreadHandler::stop()
{
  lock_guard guard(lock_);
  for (auto iter = thread_map_.begin(); iter != thread_map_.end(); ++iter) {
    Worker *worker = iter->second;
    // 这里只发停止信号；真正退出要等 worker 自己从 poll 中醒来并清理连接。
    worker->stop();
  }
  return RC::SUCCESS;
}

RC OneThreadPerConnectionThreadHandler::await_stop()
{
  LOG_INFO("begin to await stop one thread per connection thread handler");
  while (!thread_map_.empty()) {
    // 连接由各自 worker 异步关闭，这里只等待映射表清空。
    this_thread::sleep_for(chrono::milliseconds(100));
  }
  LOG_INFO("end to await stop one thread per connection thread handler");
  return RC::SUCCESS;
}
