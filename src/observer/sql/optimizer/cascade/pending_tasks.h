/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#pragma once

#include "common/lang/stack.h"
#include "common/log/log.h"
#include "sql/optimizer/cascade/tasks/cascade_task.h"

/**
 * @file pending_tasks.h
 * @brief 级联优化器待执行任务栈。
 */

/**
 * @brief This collection of undone cascade tasks is currently stored as a stack.
 */
class PendingTasks
{
public:
  /// @brief 弹出下一个待执行任务，当前实现采用后进先出策略。
  CascadeTask *pop()
  {
    auto task = task_stack_.top();
    task_stack_.pop();
    LOG_DEBUG("pop task %d", task_stack_.size());
    return task;
  }

  void push(CascadeTask *task)
  {
    task_stack_.push(task);
    LOG_DEBUG("push task %d", task_stack_.size());
  }

  bool empty() { return task_stack_.empty(); }

  ~PendingTasks()
  {
    while (!task_stack_.empty()) {
      auto task = task_stack_.top();
      task_stack_.pop();
      delete task;
    }
  }

private:
  /**
   * Stack for tracking tasks
   */
  std::stack<CascadeTask *> task_stack_;
};
