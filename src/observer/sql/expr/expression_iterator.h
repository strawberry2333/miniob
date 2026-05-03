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
// Created by Wangyunlai on 2024/05/30.
//

#pragma once

#include "common/sys/rc.h"
#include "common/lang/functional.h"
#include "common/lang/memory.h"

class Expression;

/**
 * @file expression_iterator.h
 * @brief 表达式树的一层子节点遍历工具。
 * @details 优化器与绑定逻辑会频繁需要“拿到当前节点的直接孩子并做统一处理”，
 * 这个辅助类把不同表达式类型的子节点访问方式收敛到一个入口。
 */

/**
 * @brief 为表达式的直接孩子提供统一迭代入口。
 */
class ExpressionIterator
{
public:
  /**
   * @brief 访问 `expr` 的所有直接子表达式。
   * @param expr 当前待展开的表达式节点。
   * @param callback 对每个直接孩子执行的回调。
   * @details 本函数只展开一层，递归策略由调用方自行控制。
   */
  static RC iterate_child_expr(Expression &expr, function<RC(unique_ptr<Expression> &)> callback);
};
