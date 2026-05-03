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
// Created by Meiyi & Wangyunlai on 2021/5/12.
//

#pragma once

#include "common/sys/rc.h"
#include "common/lang/string.h"
#include "sql/parser/parse_defs.h"

namespace Json {
class Value;
}  // namespace Json

/**
 * @file field_meta.h
 * @brief 定义字段元数据及其序列化接口。
 */

/**
 * @brief 描述一列在记录中的逻辑和物理布局。
 * @details `FieldMeta` 同时承担两类职责：
 * 1. 运行时描述字段名称、类型、偏移与长度；
 * 2. 作为表元数据的一部分持久化到 JSON。
 */
class FieldMeta
{
public:
  FieldMeta();
  FieldMeta(const char *name, AttrType attr_type, int attr_offset, int attr_len, bool visible, int field_id);
  ~FieldMeta() = default;

  /**
   * @brief 初始化字段元数据。
   * @details 会校验名称、类型和物理布局参数；失败时不会写入不完整状态。
   */
  RC init(const char *name, AttrType attr_type, int attr_offset, int attr_len, bool visible, int field_id);

public:
  const char *name() const;
  AttrType    type() const;
  int         offset() const;
  int         len() const;
  bool        visible() const;
  int         field_id() const;

public:
  /// @brief 按可读格式输出字段描述，供日志和调试使用。
  void desc(ostream &os) const;

public:
  /// @brief 序列化到 JSON 元数据对象。
  void      to_json(Json::Value &json_value) const;
  /// @brief 从 JSON 元数据对象恢复字段定义；格式不合法时返回错误。
  static RC from_json(const Json::Value &json_value, FieldMeta &field);

protected:
  string   name_;
  AttrType attr_type_;
  int      attr_offset_;
  int      attr_len_;
  bool     visible_;
  int      field_id_;
};
