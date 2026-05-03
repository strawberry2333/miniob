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

#include "sql/operator/physical_operator.h"
#include "sql/parser/parse.h"

/**
 * @file hash_join_physical_operator.h
 * @brief Hash Join 占位类型。
 * @details 计划生成器已经预留了 hash join 开关，但当前实现仍未落地，
 * 因此这里只保留一个最小占位声明，供后续扩展。
 */

/**
 * @brief Hash Join 算子
 * @ingroup PhysicalOperator
 */
class HashJoinPhysicalOperator
{};
