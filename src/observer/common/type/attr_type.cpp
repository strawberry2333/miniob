/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */


#include "common/lang/string.h"
#include "common/type/attr_type.h"

/**
 * @brief AttrType 的字符串映射实现。
 * @details 提供类型枚举与字符串之间的双向转换，以及若干常用类型分类判断。
 */

const char *ATTR_TYPE_NAME[] = {"undefined", "chars", "ints", "floats", "vectors", "booleans"};

/**
 * @brief 把枚举类型转换成字符串。
 * @param type 目标类型枚举。
 * @return 对应的字符串名；越界时返回 `"unknown"`。
 */
const char *attr_type_to_string(AttrType type)
{
  if (type >= AttrType::UNDEFINED && type < AttrType::MAXTYPE) {
    return ATTR_TYPE_NAME[static_cast<int>(type)];
  }
  return "unknown";
}

/**
 * @brief 从字符串解析出 AttrType。
 * @param s 类型字符串。
 * @return 对应枚举；找不到时返回 UNDEFINED。
 */
AttrType attr_type_from_string(const char *s)
{
  for (unsigned int i = 0; i < sizeof(ATTR_TYPE_NAME) / sizeof(ATTR_TYPE_NAME[0]); i++) {
    // 使用大小写无关比较，方便同时兼容配置和 SQL 层的多种输入风格。
    if (0 == strcasecmp(ATTR_TYPE_NAME[i], s)) {
      return (AttrType)i;
    }
  }
  return AttrType::UNDEFINED;
}

/// @brief 判断某个类型是否可参与标准数值运算。
bool is_numerical_type(AttrType type)
{
  return (type == AttrType::INTS || type == AttrType::FLOATS);
}

/// @brief 判断某个类型是否按字符串语义处理。
bool is_string_type(AttrType type)
{
  return (type == AttrType::CHARS);
}
