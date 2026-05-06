/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

// oblsm CLI 的词法/语法解析器。
// 这里不直接执行命令，而是把输入文本标准化成结构化结果，
// 供上层主循环决定具体调用哪个 ObLsm API。
#pragma once

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <map>
#include <ctime>

#include "common/sys/rc.h"
#include "common/lang/string.h"
#include "common/lang/string_view.h"
#include "oblsm/client/cliutil/defs.h"

#define MAX_MEM_BUFFER_SIZE 8192
namespace oceanbase {

inline const string LINE_HISTORY_FILE = "./.oblsm_cli.history";

// 当前 CLI 语法只需要区分 4 类 token：
// - COMMAND: 命令关键字，例如 open/set/get
// - STRING:  使用双引号包裹的字符串参数
// - BOUND:   scan 命令里的 `-`，表示使用首/尾边界
// - INVALID: 非法 token
enum class TokenType
{
  COMMAND,
  STRING,
  BOUND,
  INVALID
};

class ObLsmCliCmdTokenizer
{
public:
  TokenType       token_type;
  ObLsmCliCmdType cmd;
  string          str;

  ObLsmCliCmdTokenizer()
  {
    // 构造时预先建立字符串命令到枚举值的映射，解析阶段就不需要反复初始化。
#define MAP_COMMAND(cmd) token_map_[string{ObLsmCliUtil::strcmd(ObLsmCliCmdType::cmd)}] = ObLsmCliCmdType::cmd
    MAP_COMMAND(OPEN);
    MAP_COMMAND(CLOSE);
    MAP_COMMAND(SET);
    MAP_COMMAND(DELETE);
    MAP_COMMAND(SCAN);
    MAP_COMMAND(HELP);
    MAP_COMMAND(EXIT);
    MAP_COMMAND(GET);
#undef MAP_COMMAND
  }

  void init(string_view command)
  {
    // 每次解析新命令前重置扫描游标。
    command_ = command;
    p_       = 0;
  }

  RC next();

private:
  bool out_of_range() { return p_ >= command_.size(); }
  void skip_blank_space();
  // 解析带双引号的字符串字面量，支持 `\"` 转义。
  RC   parse_string(string &res);

  std::map<string, ObLsmCliCmdType> token_map_;

  string_view command_;
  size_t      p_;
};

// Parser 在 tokenizer 之上再做一层命令级语义约束检查，
// 例如 `set` 必须跟两个字符串参数，`scan` 必须跟两个边界描述。
class ObLsmCliCmdParser
{
public:
  struct Result
  {
    // 最终识别出的命令类型。
    ObLsmCliCmdType cmd;
    // 语法错误时返回给用户的提示信息。
    string          error;

    // CLI 目前最多只需要两个参数。
    string args[2];
    // 仅供 `scan` 使用，标记对应参数是否为全局边界 `-`。
    bool   bounds[2] = {false, false};
  };
  Result result;
  RC     parse(string_view command);

private:
  ObLsmCliCmdTokenizer tokenizer_;
};

}  // namespace oceanbase
