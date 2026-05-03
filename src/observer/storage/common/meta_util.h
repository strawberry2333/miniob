/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

// Created by wangyunlai.wyl on 2021/5/18.
//

#pragma once

#include "common/lang/string.h"

/**
 * @file meta_util.h
 * @brief 统一维护数据库、表、索引和 LOB 文件的命名规则。
 * @details 上层模块通过这些辅助函数拼接路径，避免各处硬编码后缀约定。
 */

static constexpr const char *DB_META_SUFFIX          = ".db";
static constexpr const char *TABLE_META_SUFFIX       = ".table";
static constexpr const char *TABLE_META_FILE_PATTERN = ".*\\.table$";
static constexpr const char *TABLE_DATA_SUFFIX       = ".data";
static constexpr const char *TABLE_INDEX_SUFFIX      = ".index";
static constexpr const char *TABLE_LOB_SUFFIX        = ".lob";

/// @brief 生成数据库元数据文件路径。
string db_meta_file(const char *base_dir, const char *db_name);
/// @brief 生成表元数据文件路径。
string table_meta_file(const char *base_dir, const char *table_name);
/// @brief 生成表数据文件路径。
string table_data_file(const char *base_dir, const char *table_name);
/// @brief 生成索引数据文件路径。
string table_index_file(const char *base_dir, const char *table_name, const char *index_name);
/// @brief 生成 LOB 数据文件路径。
string table_lob_file(const char *base_dir, const char *table_name);
