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
// Created by Wangyunlai on 2023/6/14.
//

#pragma once

#include "common/sys/rc.h"
#include "event/session_event.h"
#include "event/sql_event.h"
#include "sql/executor/sql_result.h"
#include "sql/operator/string_list_physical_operator.h"

/**
 * @file help_executor.h
 * @brief 定义 `HELP` 命令执行器。
 */

/**
 * @brief 输出内置帮助信息的命令执行器。
 * @ingroup Executor
 */
class HelpExecutor
{
public:
  HelpExecutor()          = default;
  virtual ~HelpExecutor() = default;

  /**
   * @brief 生成一张只读帮助结果表。
   * @param sql_event 当前 SQL 请求上下文。
   * @return 总是返回 `RC::SUCCESS`。
   */
  RC execute(SQLStageEvent *sql_event)
  {
    // 这里维护的是一个纯文本帮助列表，每一项会输出成结果集中的一行。
    const char *strings[] = {"show tables;",
        "desc `table name`;",
        "create table `table name` (`column name` `column type`, ...);",
        "create index `index name` on `table` (`column`);",
        "insert into `table` values(`value1`,`value2`);",
        "update `table` set column=value [where `column`=`value`];",
        "delete from `table` [where `column`=`value`];",
        "select [ * | `columns` ] from `table`;"};

    auto oper = new StringListPhysicalOperator();
    // 逐条填充帮助文本，后续由结果集统一迭代输出。
    for (size_t i = 0; i < sizeof(strings) / sizeof(strings[0]); i++) {
      oper->append(strings[i]);
    }

    SqlResult *sql_result = sql_event->session_event()->sql_result();

    // 帮助结果只有一列，列名固定为 `Commands`。
    TupleSchema schema;
    schema.append_cell("Commands");

    sql_result->set_tuple_schema(schema);
    sql_result->set_operator(unique_ptr<PhysicalOperator>(oper));

    return RC::SUCCESS;
  }
};
