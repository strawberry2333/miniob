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
// Created by Wangyunlai on 2022/11/17.
//

#include "net/communicator.h"
#include "net/buffered_writer.h"
#include "net/cli_communicator.h"
#include "net/mysql_communicator.h"
#include "net/plain_communicator.h"
#include "session/session.h"

#include "common/lang/mutex.h"

/**
 * @file communicator.cpp
 * @brief Communicator 基类和协议工厂的实现。
 */

RC Communicator::init(int fd, unique_ptr<Session> session, const string &addr)
{
  // 连接级资源在这里统一落地，派生类只补充协议相关初始化逻辑。
  fd_      = fd;
  session_ = std::move(session);
  addr_    = addr;
  writer_  = new BufferedWriter(fd_);  // 所有协议共享同一套缓冲写路径。
  return RC::SUCCESS;
}

Communicator::~Communicator()
{
  if (fd_ >= 0) {
    // 默认 communicator 拥有网络 fd；CLI 模式会把 fd_ 改成 -1 以跳过这里。
    close(fd_);
    fd_ = -1;
  }

  if (writer_ != nullptr) {
    // BufferedWriter::close 只会刷缓存，不会重复关闭底层 fd。
    delete writer_;
    writer_ = nullptr;
  }
}

/////////////////////////////////////////////////////////////////////////////////

Communicator *CommunicatorFactory::create(CommunicateProtocol protocol)
{
  // 工厂只负责根据协议类型挑选具体实现，连接级初始化在 accept 后完成。
  switch (protocol) {
    case CommunicateProtocol::PLAIN: {
      return new PlainCommunicator;
    } break;
    case CommunicateProtocol::CLI: {
      return new CliCommunicator;
    } break;
    case CommunicateProtocol::MYSQL: {
      return new MysqlCommunicator;
    } break;
    default: {
      return nullptr;
    }
  }
}
