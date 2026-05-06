/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

// CLI 命令元信息定义。
// 这里使用 X-Macro 同时生成命令枚举、命令字符串和帮助文案，
// 避免多处维护时发生遗漏或不一致。
#include "common/lang/string_view.h"
namespace oceanbase {

/**
 * @brief 声明一组 oblsm CLI 命令。
 *
 * 如需扩展新命令，只需要在这里增加一行 `DEFINE_OBLSM_CLI_CMD`，
 * 下面的枚举和辅助函数会自动复用这份定义。
 */
#define DEFINE_OBLSM_CLI_CMDS                                                                                          \
  DEFINE_OBLSM_CLI_CMD(                                                                                                \
      OPEN, "open", "Usage: open \"DATABASE_NAME\"\nOpens a database. If the database does not exist, it is created.") \
  DEFINE_OBLSM_CLI_CMD(CLOSE, "close", "Usage: close\nCloses the currently opened database.")                          \
  DEFINE_OBLSM_CLI_CMD(SET, "set", "Usage: set \"KEY\" \"VALUE\"\nSets the value for the given key.")                  \
  DEFINE_OBLSM_CLI_CMD(GET, "get", "Usage: get \"KEY\"\nGets the value associated with the given key.")                \
  DEFINE_OBLSM_CLI_CMD(DELETE, "delete", "Usage: delete \"KEY\"\nDeletes the given key.")                              \
  DEFINE_OBLSM_CLI_CMD(SCAN,                                                                                           \
      "scan",                                                                                                          \
      "Usage: scan \"KEY1\" \"KEY2\"\n"                                                                                \
      "Scans the database for keys within the specified range.\n"                                                      \
      "The first and second keys can be specified in the following ways:\n"                                            \
      "- 'scan - \"KEY\"' scans from the first key in the database up to the specified key \"KEY\".\n"                 \
      "- 'scan \"KEY\" -' scans from the specified key \"KEY\" to the last key in the database.\n"                     \
      "- 'scan - -' scans the entire database from the first key to the last key.\n"                                   \
      "- 'scan \"KEY1\" \"KEY2\"' scans from the key \"KEY1\" to the key \"KEY2\".\n")                                 \
  DEFINE_OBLSM_CLI_CMD(HELP, "help", "")                                                                               \
  DEFINE_OBLSM_CLI_CMD(EXIT, "exit", "Usage: exit\nExits the application.")

enum class ObLsmCliCmdType
{
#define DEFINE_OBLSM_CLI_CMD(cmd, str, help) cmd,
  DEFINE_OBLSM_CLI_CMDS
#undef DEFINE_OBLSM_CLI_CMD
};

struct ObLsmCliUtil
{
  // 把命令枚举转回终端输入时使用的关键字字符串。
  inline static const string_view strcmd(ObLsmCliCmdType command)
  {
#define DEFINE_OBLSM_CLI_CMD(cmd, name, help) \
  case ObLsmCliCmdType::cmd: {                \
    return name;                              \
  } break;

    switch (command) {
      DEFINE_OBLSM_CLI_CMDS;
      default: {
        return "unknown";
      }
    }

#undef DEFINE_OBLSM_CLI_CMD
  }

  // 返回对应命令的 usage 文案，用于 help 和报错提示。
  inline static const string_view cmd_usage(ObLsmCliCmdType command)
  {
#define DEFINE_OBLSM_CLI_CMD(cmd, name, help) \
  case ObLsmCliCmdType::cmd: {                \
    return help;                              \
  } break;

    switch (command) {
      DEFINE_OBLSM_CLI_CMDS;
      default: {
        return "Unknown command usage.";
      }
    }

#undef DEFINE_OBLSM_CLI_CMD
  }
};
}  // namespace oceanbase
