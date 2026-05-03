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
// Created by Wangyunlai on 2022/12/15
//

#include "sql/operator/table_get_logical_operator.h"
#include "sql/optimizer/cascade/property.h"
#include "catalog/catalog.h"

/**
 * @file table_get_logical_operator.cpp
 * @brief 逻辑取表节点的属性与谓词绑定实现。
 */

TableGetLogicalOperator::TableGetLogicalOperator(Table *table, ReadWriteMode mode)
    : LogicalOperator(), table_(table), mode_(mode)
{}

void TableGetLogicalOperator::set_predicates(vector<unique_ptr<Expression>> &&exprs)
{
  predicates_ = std::move(exprs);
}

unique_ptr<LogicalProperty> TableGetLogicalOperator::find_log_prop(const vector<LogicalProperty*> &log_props)
{
  int card = Catalog::get_instance().get_table_stats(table_->table_id()).row_nums;
  // 当前只使用表级统计信息，尚未根据下推谓词进一步修正基数。
  // TODO: think about predicates.
  return make_unique<LogicalProperty>(card);
}
