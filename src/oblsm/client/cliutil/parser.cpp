/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

// CLI 解析实现分为两层：
// - tokenizer: 负责切 token
// - parser:    负责按命令格式消费 token 并做约束校验
#include "oblsm/client/cliutil/parser.h"
#include "common/lang/string.h"
#include "common/log/log.h"
#include "common/sys/rc.h"

namespace oceanbase {
void ObLsmCliCmdTokenizer::skip_blank_space()
{
  // CLI 允许 token 之间有任意数量空白字符。
  while (!out_of_range() && std::isspace(command_[p_])) {
    p_++;
  }
}

RC ObLsmCliCmdTokenizer::parse_string(string &res)
{
  // 当前字符串语法非常轻量：以 `"` 开始，以未转义 `"` 结束。
  while (!out_of_range() && command_[p_] != '"') {
    char ch = command_[p_++];
    if (ch == '\\' && !out_of_range() && command_[p_] == '"') {
      // 只处理 `\"` 这一种转义，足够覆盖带引号的 key/value。
      res.push_back('"');
      p_++;
    } else {
      res.push_back(ch);
    }
  }

  if (out_of_range()) {
    return RC::UNEXPECTED_END_OF_STRING;
  }
  p_++;

  return RC::SUCCESS;
}

RC ObLsmCliCmdTokenizer::next()
{
  RC     rc = RC::SUCCESS;
  string current;

  skip_blank_space();
  if (out_of_range()) {
    return RC::INPUT_EOF;
  }

  // 例如 `scan - "key"` 中的 `-`，表示使用数据库最小/最大边界。
  if (command_.at(p_) == '-') {
    token_type = TokenType::BOUND;
    p_++;
    return rc;
  }

  bool is_str = command_.at(p_) == '\"';

  if (is_str) {
    token_type = TokenType::STRING;
    p_++;
    rc  = parse_string(current);
    str = std::move(current);
    if (rc != RC::SUCCESS) {
      token_type = TokenType::INVALID;
    }
    return rc;
  }

  token_type = TokenType::COMMAND;
  while (!out_of_range()) {
    char ch = command_.at(p_++);
    if (std::isspace(ch) || ch == '\"') {
      // 命令关键字用空白或引号结束；引号通常意味着下一个 token 是字符串参数。
      break;
    }
    current.push_back(ch);
  }

  auto iter = token_map_.find(current);
  if (iter == token_map_.end()) {
    token_type = TokenType::INVALID;
    return RC::INVALID_TOKEN;
  }
  cmd = iter->second;

  return rc;
}

RC ObLsmCliCmdParser::parse(string_view command)
{
  // 解析入口假设一行只包含一条命令，不做多语句拆分。
  tokenizer_.init(command);

  RC rc = tokenizer_.next();
  if (OB_FAIL(rc)) {
    return rc;
  }

  if (tokenizer_.token_type != TokenType::COMMAND) {
    return RC::SYNTAX_ERROR;
  }
  result.cmd = tokenizer_.cmd;

  switch (tokenizer_.cmd) {
    case ObLsmCliCmdType::OPEN:
      rc = tokenizer_.next();
      if (OB_FAIL(rc) || tokenizer_.token_type != TokenType::STRING) {
        // 语法错误时把对应命令的 usage 填入结果，供上层直接打印。
        result.error = ObLsmCliUtil::cmd_usage(ObLsmCliCmdType::OPEN);
        return RC::SYNTAX_ERROR;
      }
      result.args[0] = tokenizer_.str;
      break;
    case ObLsmCliCmdType::SET:
      for (size_t i = 0; i < 2; ++i) {
        rc = tokenizer_.next();
        if (OB_FAIL(rc) || tokenizer_.token_type != TokenType::STRING) {
          result.error = ObLsmCliUtil::cmd_usage(ObLsmCliCmdType::SET);
          return RC::SYNTAX_ERROR;
        }
        result.args[i] = std::move(tokenizer_.str);
      }
      break;
    case ObLsmCliCmdType::GET:
      rc = tokenizer_.next();
      if (OB_FAIL(rc) || tokenizer_.token_type != TokenType::STRING) {
        result.error = ObLsmCliUtil::cmd_usage(ObLsmCliCmdType::GET);
        return RC::SYNTAX_ERROR;
      }
      result.args[0] = tokenizer_.str;
      break;
    case ObLsmCliCmdType::DELETE:
      rc = tokenizer_.next();
      if (OB_FAIL(rc) || tokenizer_.token_type != TokenType::STRING) {
        result.error = ObLsmCliUtil::cmd_usage(ObLsmCliCmdType::DELETE);
        return RC::SYNTAX_ERROR;
      }
      result.args[0] = tokenizer_.str;
      break;
    case ObLsmCliCmdType::SCAN:
      for (size_t i = 0; i < 2; ++i) {
        rc = tokenizer_.next();
        if (OB_FAIL(rc)) {
          result.error = ObLsmCliUtil::cmd_usage(ObLsmCliCmdType::SCAN);
          return RC::SYNTAX_ERROR;
        }

        if (tokenizer_.token_type == TokenType::BOUND) {
          // `-` 表示该方向使用全局边界，而不是具体 key。
          result.bounds[i] = true;
        } else if (tokenizer_.token_type == TokenType::STRING) {
          result.args[i] = std::move(tokenizer_.str);
        } else {
          result.error = ObLsmCliUtil::cmd_usage(ObLsmCliCmdType::SCAN);
          return RC::SYNTAX_ERROR;
        }
      }
      break;
    case ObLsmCliCmdType::HELP:

    case ObLsmCliCmdType::EXIT:
    case ObLsmCliCmdType::CLOSE: break;
    default: ASSERT(false, "unimplement!"); break;
  }

  return rc;
}

}  // namespace oceanbase
