/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "sql/optimizer/cascade/rules.h"
#include "sql/optimizer/cascade/implementation_rules.h"
#include "sql/optimizer/cascade/group_expr.h"

/**
 * @file rules.cpp
 * @brief 默认规则集注册逻辑。
 */

RuleSet::RuleSet()
{
  // 当前仅注册 logical->physical 的实现规则；逻辑变换规则尚未接入。
  add_rule(RuleSetName::PHYSICAL_IMPLEMENTATION, new LogicalProjectionToProjection());
  add_rule(RuleSetName::PHYSICAL_IMPLEMENTATION, new LogicalGetToPhysicalSeqScan());
  add_rule(RuleSetName::PHYSICAL_IMPLEMENTATION, new LogicalInsertToInsert());
  add_rule(RuleSetName::PHYSICAL_IMPLEMENTATION, new LogicalExplainToExplain());
  add_rule(RuleSetName::PHYSICAL_IMPLEMENTATION, new LogicalCalcToCalc());
  add_rule(RuleSetName::PHYSICAL_IMPLEMENTATION, new LogicalDeleteToDelete());
  add_rule(RuleSetName::PHYSICAL_IMPLEMENTATION, new LogicalPredicateToPredicate());
}
