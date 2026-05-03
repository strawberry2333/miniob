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

#include "sql/operator/operator_node.h"
#include "common/lang/vector.h"
#include "common/lang/memory.h"

/**
 * @file pattern.h
 * @brief 规则匹配模式树。
 */

/**
 * @brief 描述一条规则期望匹配到的算子形状。
 */
class Pattern
{
public:
  explicit Pattern(OpType op) : type_(op) {}
  ~Pattern() {}

  /// @brief 追加一个子模式，形成与算子树同构的匹配结构。
  void add_child(Pattern *child) { children_.push_back(unique_ptr<Pattern>(child)); }

  const vector<unique_ptr<Pattern>> &children() const { return children_; }

  size_t get_child_patterns_size() const { return children_.size(); }

  OpType type() const { return type_; }

private:
  OpType type_;

  vector<unique_ptr<Pattern>> children_;
};
