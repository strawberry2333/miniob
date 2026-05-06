/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */
// oblsm 命令行工具入口。
// 这个程序本身不是存储引擎的一部分，但它把用户命令映射到 ObLsm API，
// 因此是理解模块对外使用方式的最直接样例。

#include "common/lang/filesystem.h"
#include "common/sys/rc.h"
#include "oblsm/client/cliutil/parser.h"
#include "oblsm/include/ob_lsm.h"
#include "oblsm/include/ob_lsm_iterator.h"
#include "oblsm/include/ob_lsm_options.h"
#include "oblsm/util/ob_comparator.h"
#include "common/linereader/line_reader.h"

using namespace oceanbase;
using common::MiniobLineReader;

const string prompt = "\033[32moblsm> \033[0m";
bool         quit   = false;
ObLsm       *lsm    = nullptr;
// 客户端当前直接使用默认配置打开数据库，便于聚焦命令演示而不是调参入口。
ObLsmOptions opt;

const char *startup_tips = R"(
Welcome to the OceanBase database implementation course.

Copyright (c) 2021 OceanBase and/or its affiliates.

Learn more about OceanBase at https://github.com/oceanbase/oceanbase
Learn more about MiniOB at https://github.com/oceanbase/miniob

)";

void print_rc(RC rc)
{
  // 使用红色高亮错误码，便于在交互模式下快速定位失败操作。
  std::cout << "\033[31m";
  std::cout << "rc, " << strrc(rc) << std::endl;
  // 恢复终端默认颜色，避免污染后续输出。
  std::cout << "\033[0m";
}

void print_sys_msg(string_view msg)
{
  // 系统提示统一使用绿色，和错误输出形成区分。
  std::cout << "\033[32m";
  std::cout << msg << std::endl;
  // 恢复终端默认颜色。
  std::cout << "\033[0m";
}

std::vector<std::pair<string, string>> scan(
    const std::string *strs, const bool *bounds, ObDefaultComparator &comparator)
{
  // `scan` 通过构造两个迭代器来表示区间的左右边界：
  // - runner: 当前扫描游标
  // - end:    区间右端点
  // bounds[i] == true 表示该端使用全局边界，而不是具体 key。
  std::vector<std::pair<string, string>> res;

  if (!bounds[0] && !bounds[1] && comparator.compare(strs[0], strs[1]) > 0) {
    // 左边界大于右边界时直接返回空结果，避免无意义扫描。
    return res;
  }
  ObLsmReadOptions rd_opt;
  auto             runner = std::unique_ptr<ObLsmIterator>(lsm->new_iterator(rd_opt));
  auto             end    = std::unique_ptr<ObLsmIterator>(lsm->new_iterator(rd_opt));

  if (bounds[0]) {
    runner->seek_to_first();
  } else {
    runner->seek(strs[0]);
  }

  if (!runner->valid()) {
    return res;
  }

  if (bounds[1]) {
    end->seek_to_last();
  } else {
    end->seek(strs[1]);
    if (!end->valid()) {
      // 如果右边界 key 不存在，则退化为“扫描到最后一个实际 key”。
      end->seek_to_last();
    }
  }

  if (!end->valid()) {
    return res;
  }

  while (runner->valid() && comparator.compare(runner->key(), end->key()) <= 0) {
    res.emplace_back(runner->key(), runner->value());
    runner->next();
  }

  return res;
}

void help()
{
  // `help` 直接枚举命令元信息，避免手写重复文案。
  for (int i = static_cast<int>(ObLsmCliCmdType::OPEN); i <= static_cast<int>(ObLsmCliCmdType::EXIT); ++i) {
    ObLsmCliCmdType cmd = static_cast<ObLsmCliCmdType>(i);
    if (cmd != ObLsmCliCmdType::HELP) {
      std::cout << "Command: " << ObLsmCliUtil::strcmd(cmd) << std::endl;
      std::cout << ObLsmCliUtil::cmd_usage(cmd) << std::endl;
      std::cout << std::endl;
    }
  }
}

int main(int, char **)
{
  // 交互主循环负责三件事：
  // 1. 读取命令行输入；
  // 2. 调用 parser 做语法解析；
  // 3. 把解析结果转成具体的存储引擎 API 调用。
  print_sys_msg(startup_tips);
  print_sys_msg("Enter the help command to view the usage of oblsm_cli");

  MiniobLineReader::instance().init(LINE_HISTORY_FILE);

  for (; !quit;) {
    std::string command_input = MiniobLineReader::instance().my_readline(prompt.c_str());

    if (command_input.empty()) {
      continue;
    }

    ObLsmCliCmdParser parser;
    auto            &&result     = parser.result;
    RC                rc         = parser.parse(command_input);
    auto              comparator = ObDefaultComparator{};
    if (OB_FAIL(rc)) {
      print_rc(rc);
      print_sys_msg(result.error);
      continue;
    }

    if (result.cmd != ObLsmCliCmdType::EXIT && result.cmd != ObLsmCliCmdType::OPEN &&
        result.cmd != ObLsmCliCmdType::HELP && lsm == nullptr) {
      // 除 `open/help/exit` 外，其余命令都依赖已打开的数据库句柄。
      print_sys_msg("Please open a database first!");
      continue;
    }

    switch (result.cmd) {
      case ObLsmCliCmdType::OPEN:
        if (!filesystem::exists(result.args[0])) {
          // CLI 体验上允许“目录不存在则自动创建”，减少首次使用门槛。
          filesystem::create_directory(result.args[0]);
        }
        if (lsm) {
          delete lsm;
          lsm = nullptr;
        }
        rc = ObLsm::open(opt, result.args[0], &lsm);
        if (OB_FAIL(rc)) {
          print_rc(rc);
        }
        break;
      case ObLsmCliCmdType::SET:
        rc = lsm->put(result.args[0], result.args[1]);
        if (OB_FAIL(rc)) {
          print_rc(rc);
        }
        break;
      case ObLsmCliCmdType::GET: {
        ObLsmReadOptions rd_opt;

        auto seek_iter = std::unique_ptr<ObLsmIterator>(lsm->new_iterator(rd_opt));
        seek_iter->seek(result.args[0]);
        if (seek_iter->valid() && comparator.compare(seek_iter->key(), result.args[0]) == 0) {
          // 只有命中同名 key 才输出 value；seek 到下一个更大 key 不算命中。
          std::cout << seek_iter->value() << std::endl;
        }
      } break;
      case ObLsmCliCmdType::DELETE: {
        print_sys_msg("unimplement!");
        break;
      }
      case ObLsmCliCmdType::SCAN: {
        auto scan_res = scan(result.args, result.bounds, comparator);
        for (const auto &[k, v] : scan_res) {
          std::cout << k << " " << v << "\n";
        }
        std::cout << std::endl;
      } break;
      case ObLsmCliCmdType::CLOSE:
        delete lsm;
        lsm = nullptr;
        print_sys_msg("Success.");
        break;
      case ObLsmCliCmdType::HELP: help(); break;
      case ObLsmCliCmdType::EXIT:
        if (lsm) {
          delete lsm;
          lsm = nullptr;
        }
        print_sys_msg("bye.");
        quit = true;
        break;
    }
  }
}
