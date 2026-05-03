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
// Created by Wangyunlai on 2022/07/05.
//

#pragma once

#include "storage/field/field_meta.h"
#include "storage/table/table.h"

/**
 * @file field.h
 * @brief 定义绑定到具体表元数据的字段访问器。
 */

/**
 * @brief 表字段访问器。
 * @details `Field` 把 `Table` 与 `FieldMeta` 绑定在一起，方便上层根据字段定义读写 Record。
 * 当前只提供少量按偏移访问的便捷接口，不负责任何并发控制或类型转换策略。
 */
class Field
{
public:
  Field() = default;
  Field(const Table *table, const FieldMeta *field) : table_(table), field_(field) {}
  Field(const Field &) = default;

  const Table     *table() const { return table_; }
  const FieldMeta *meta() const { return field_; }

  AttrType attr_type() const { return field_->type(); }

  const char *table_name() const { return table_->name(); }
  const char *field_name() const { return field_->name(); }

  void set_table(const Table *table) { this->table_ = table; }
  void set_field(const FieldMeta *field) { this->field_ = field; }

  /**
   * @brief 向记录中写入一个整数字段。
   * @details 调用方必须保证目标记录拥有可写内存，且字段类型确实是 `INTS`。
   */
  void set_int(Record &record, int value);
  /**
   * @brief 从记录中读取整数字段。
   * @details 实际读取仍经由 `Value` 解释，便于复用类型系统的解码逻辑。
   */
  int  get_int(const Record &record);

  /**
   * @brief 获取字段在记录中的原始数据指针。
   * @details 返回值直接指向 Record 当前数据区，不做拷贝；调用方需要保证 Record 生命周期和并发访问安全。
   */
  const char *get_data(const Record &record);

private:
  const Table     *table_ = nullptr;
  const FieldMeta *field_ = nullptr;
};
