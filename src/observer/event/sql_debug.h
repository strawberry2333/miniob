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
// Created by Wangyunlai on 2023/6/29.
//

#pragma once

#include "common/lang/string.h"
#include "common/lang/list.h"

/**
 * @brief SQL 调试输出接口定义。
 * @details 该文件提供单条 SQL 的调试信息容器，以及一个基于当前线程 Session 的便捷追加入口。
 */

/**
 * @brief SQL调试信息
 * @details
 * 希望在运行SQL时，可以直接输出一些调试信息到客户端。
 * 当前把调试信息都放在了session上，可以随着SQL语句输出。
 * 但是现在还不支持与输出调试信息与行数据同步输出。
 */
class SqlDebug
{
public:
  SqlDebug()          = default;
  virtual ~SqlDebug() = default;

  /// @brief 追加一条调试文本，按生成顺序保存在链表尾部。
  void add_debug_info(const string &debug_info);
  /// @brief 清空当前请求累计的调试输出。
  void clear_debug_info();

  /// @brief 返回内部调试文本集合的只读引用。
  const list<string> &get_debug_infos() const;

private:
  list<string> debug_infos_;
};

/**
 * @brief 增加SQL的调试信息
 * @details 可以在任何执行SQL语句时调用这个函数来增加调试信息。
 * 如果当前上下文不在SQL执行过程中，那么不会生成调试信息。
 * 在普通文本场景下，调试信息会直接输出到客户端，并增加 '#' 作为前缀。
 * @param fmt printf 风格格式串。
 * @note 该函数依赖当前线程已经绑定 Session 和当前请求。
 */
void sql_debug(const char *fmt, ...);
