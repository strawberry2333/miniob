/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

// __CR__

/*
 *  Created on: Mar 11, 2012
 *      Author: Longda Feng
 */

#include <netinet/in.h>
#include <unistd.h>

#include "common/ini_setting.h"
#include "common/init.h"
#include "common/lang/iostream.h"
#include "common/lang/string.h"
#include "common/lang/map.h"
#include "common/os/process.h"
#include "common/os/signal.h"
#include "common/log/log.h"
#include "net/server.h"
#include "net/server_param.h"

using namespace common;

// observer.ini 中网络监听配置所在的 section 名字。
#define NET "NET"

/**
 * 这个文件是 observer 进程的程序入口。
 *
 * 可以把它理解成 MiniOB 服务端的“装配层”：
 * 1. 打印启动提示；
 * 2. 安装退出信号处理逻辑；
 * 3. 解析命令行参数，填充到全局 ProcessParam 中；
 * 4. 调用全局 init() 完成日志、配置、存储等模块初始化；
 * 5. 根据配置决定创建哪一种 Server（网络模式或 CLI 模式）；
 * 6. 调用 serve() 进入主循环，阻塞等待请求；
 * 7. 收到退出信号或服务结束后，执行 cleanup() 收尾。
 *
 * 这个文件本身不负责 SQL 解析、执行、存储实现，它只负责“把系统启动起来”。
 */
// 当前进程正在运行的服务端对象。
// main() 负责创建和销毁它；信号处理线程只借用这个指针触发 shutdown。
static Server *g_server = nullptr;

/**
 * 打印命令行帮助。
 *
 * 这里列出的参数，最终都会写入 ProcessParam，后续 init() 和 init_server()
 * 会根据这些参数决定：
 * - 用哪个配置文件；
 * - 监听哪个端口；
 * - 使用什么协议；
 * - 使用什么事务/存储/线程模型。
 */
void usage()
{
  cout << "Usage " << endl;
  cout << "-p: server port. if not specified, the item in the config file will be used" << endl;
  cout << "-f: path of config file." << endl;
  cout << "-s: use unix socket and the argument is socket address" << endl;
  cout << "-P: protocol. {plain(default), mysql, cli}." << endl;
  cout << "-t: transaction model. {vacuous(default), mvcc}." << endl;
  cout << "-T: thread handling model. {one-thread-per-connection(default),java-thread-pool}." << endl;
  cout << "-n: buffer pool memory size in byte" << endl;
  cout << "-d: durbility mode. {vacuous(default), disk}" << endl;
  // TODO: support multi dbs(storage/db/db.h) and remove this options
  cout << "-E: storage engine. {heap(default), lsm}" << endl;
}

/**
 * 解析命令行参数，并写入全局的 ProcessParam。
 *
 * 关键点：
 * - 先调用 init_default(process_name) 设置默认值；
 * - 再用 getopt 解析命令行；
 * - 如果同一个配置项既存在于配置文件、又存在于命令行，
 *   后续一般以命令行为准。
 *
 * 例如：
 *   observer -f xxx.ini -p 6789 -P cli
 *
 * 其中：
 * -f 指定配置文件路径；
 * -p 覆盖配置文件里的监听端口；
 * -P 指定通信协议/交互方式。
 */
void parse_parameter(int argc, char **argv)
{
  // 根据 argv[0] 提取进程名，例如 observer。
  // 这个名字会被用来推导默认配置文件路径、默认日志路径。
  string process_name = get_process_name(argv[0]);

  ProcessParam *process_param = the_process_param();

  // 先写入一份“默认参数”：
  // - 默认配置文件：../etc/<process_name>.ini
  // - 默认日志输出：../log/<process_name>.out/.err
  process_param->init_default(process_name);

  // getopt 的格式串说明：
  // 冒号表示该参数后面需要带值，例如 -p 6789、-f observer.ini。
  int          opt;
  extern char *optarg;
  while ((opt = getopt(argc, argv, "dp:P:s:t:T:f:o:e:E:hn:")) > 0) {
    switch (opt) {
      // unix domain socket 路径
      case 's': process_param->set_unix_socket_path(optarg); break;
      // TCP 监听端口
      case 'p': process_param->set_server_port(atoi(optarg)); break;
      // 通信协议：plain / mysql / cli
      case 'P': process_param->set_protocol(optarg); break;
      // 配置文件路径
      case 'f': process_param->set_conf(optarg); break;
      // 标准输出/错误输出重定向文件
      case 'o': process_param->set_std_out(optarg); break;
      case 'e': process_param->set_std_err(optarg); break;
      // 事务模型、存储引擎、线程处理模型
      case 't': process_param->set_trx_kit_name(optarg); break;
      case 'E': process_param->set_storage_engine(optarg); break;
      case 'T': process_param->set_thread_handling_name(optarg); break;
      // Buffer Pool 大小
      case 'n': process_param->set_buffer_pool_memory_size(atoi(optarg)); break;
      // 持久化模式。这里只是把字符串参数记下来，真正如何生效由后续初始化阶段决定。
      case 'd': process_param->set_durability_mode("disk"); break;
      case 'h':
        usage();
        exit(0);
        return;
      default: cout << "Unknown option: " << static_cast<char>(opt) << ", ignored" << endl; break;
    }
  }
}

/**
 * 根据配置文件和命令行参数，构造真正的 Server 对象。
 *
 * 这一步的职责是把“字符串配置”翻译成“运行期对象”：
 * - 读取 [NET] 配置段；
 * - 解析监听地址、端口、最大连接数；
 * - 决定协议类型；
 * - 决定是创建 NetServer 还是 CliServer。
 *
 * 注意这里有一个优先级：
 * 默认值 < 配置文件 < 命令行参数
 */
Server *init_server()
{
  // 读取配置文件中 [NET] 这一段，返回键值对映射。
  map<string, string> net_section = get_properties()->get(NET);

  ProcessParam *process_param = the_process_param();

  // 先准备一组兜底默认值。后面如果配置文件或命令行给了值，再覆盖它们。
  long listen_addr        = INADDR_ANY;
  long max_connection_num = MAX_CONNECTION_NUM_DEFAULT;
  int  port               = PORT_DEFAULT;

  // 读取配置文件中的监听地址。
  // INADDR_ANY 表示绑定到所有网卡地址。
  map<string, string>::iterator it = net_section.find(CLIENT_ADDRESS);
  if (it != net_section.end()) {
    string str = it->second;
    str_to_val(str, listen_addr);
  }

  // 读取配置文件中的最大连接数。
  it = net_section.find(MAX_CONNECTION_NUM);
  if (it != net_section.end()) {
    string str = it->second;
    str_to_val(str, max_connection_num);
  }

  // 端口有两种来源：
  // 1. 命令行 -p：优先级最高；
  // 2. 配置文件 [NET]::PORT；
  // 如果两边都没有，就使用 PORT_DEFAULT。
  if (process_param->get_server_port() > 0) {
    port = process_param->get_server_port();
    LOG_INFO("Use port config in command line: %d", port);
  } else {
    it = net_section.find(PORT);
    if (it != net_section.end()) {
      string str = it->second;
      str_to_val(str, port);
    }
  }

  ServerParam server_param;
  server_param.listen_addr        = listen_addr;
  server_param.max_connection_num = max_connection_num;
  server_param.port               = port;

  // 根据 -P / protocol 配置选择通信方式：
  // - mysql: 走 MySQL 协议，方便用 mysql client 连接；
  // - cli:   走标准输入输出，不监听网络端口，适合本地调试；
  // - plain: MiniOB 自己的纯文本协议，也是默认模式。
  if (0 == strcasecmp(process_param->get_protocol().c_str(), "mysql")) {
    server_param.protocol = CommunicateProtocol::MYSQL;
  } else if (0 == strcasecmp(process_param->get_protocol().c_str(), "cli")) {
    // cli 模式下不走 socket，而是直接从当前终端读写。
    server_param.use_std_io = true;
    server_param.protocol   = CommunicateProtocol::CLI;
  } else {
    server_param.protocol = CommunicateProtocol::PLAIN;
  }

  // 只有非 stdio 模式下，unix socket 配置才有意义。
  // 因为 cli 模式已经直接绑定到当前进程的 stdin/stdout 了，不需要再建 socket。
  if (process_param->get_unix_socket_path().size() > 0 && !server_param.use_std_io) {
    server_param.use_unix_socket  = true;
    server_param.unix_socket_path = process_param->get_unix_socket_path();
  }

  // 线程模型只是在这里透传给 server，真正怎么创建线程由 server 内部负责。
  server_param.thread_handling = process_param->thread_handling_name();

  Server *server = nullptr;
  if (server_param.use_std_io) {
    // CLI 模式：请求来自当前终端输入。
    server = new CliServer(server_param);
  } else {
    // 网络模式：监听 TCP 或 Unix Socket，等待客户端连接。
    server = new NetServer(server_param);
  }

  return server;
}

/**
 * 如果收到terminal信号的时候，正在处理某些事情，比如打日志，并且拿着日志的锁
 * 那么直接在signal_handler里面处理的话，可能会导致死锁
 * 所以这里单独创建一个线程
 */
void *quit_thread_func(void *_signum)
{
  // signal handler 里传进来的是 int，这里借助 intptr_t 做一次安全转换。
  intptr_t signum = (intptr_t)_signum;
  LOG_INFO("Receive signal: %ld", signum);
  if (g_server) {
    // 触发服务端停止。通常会让 serve() 主循环跳出，后续 main() 继续执行清理逻辑。
    g_server->shutdown();
  }
  return nullptr;
}

/**
 * 退出信号处理入口。
 *
 * 这里故意不在 signal handler 里直接做复杂逻辑，而是：
 * 1. 先移除信号处理器，防止重复进入；
 * 2. 拉起一个普通线程；
 * 3. 在线程里调用 g_server->shutdown()。
 *
 * 原因是 signal handler 的上下文非常受限，很多常规函数并不是异步信号安全的。
 * 比如日志、锁、内存分配，都可能在这里出问题。
 */
void quit_signal_handle(int signum)
{
  // 防止多次调用退出
  // 其实正确的处理是，应该全局性的控制来防止出现“多次”退出的状态，包括发起信号
  // 退出与进程主动退出
  set_signal_handler(nullptr);

  pthread_t tid;
  pthread_create(&tid, nullptr, quit_thread_func, (void *)(intptr_t)signum);
}

// 启动时打印在终端上的欢迎信息。它只是展示文案，不参与任何业务逻辑。
const char *startup_tips = R"(
Welcome to the OceanBase database implementation course.

Copyright (c) 2021 OceanBase and/or its affiliates.

Learn more about OceanBase at https://github.com/oceanbase/oceanbase
Learn more about MiniOB at https://github.com/oceanbase/miniob

)";

/**
 * observer 主函数。
 *
 * 整体流程非常固定：
 * 1. 打印欢迎信息；
 * 2. 注册退出信号处理函数；
 * 3. 解析命令行参数；
 * 4. 调用 init() 初始化全局基础设施；
 * 5. 创建 server；
 * 6. 调用 serve() 进入主循环；
 * 7. 主循环退出后执行 cleanup()；
 * 8. 释放 server 对象并退出进程。
 */
int main(int argc, char **argv)
{
  int rc = STATUS_SUCCESS;

  // 打印启动横幅，方便确认当前启动的是哪个程序。
  cout << startup_tips;

  // 注册统一的退出信号处理逻辑，通常用于 Ctrl+C、kill 等场景。
  set_signal_handler(quit_signal_handle);

  // 解析命令行，结果会写进全局 ProcessParam。
  parse_parameter(argc, argv);

  // init() 会完成更大范围的系统初始化，例如：
  // - 读取配置文件
  // - 初始化日志
  // - 初始化存储/事务等基础模块
  //
  // main.cpp 不关心这些细节，只关心成败。
  rc = init(the_process_param());
  if (rc != STATUS_SUCCESS) {
    cerr << "Shutdown due to failed to init!" << endl;
    cleanup();
    return rc;
  }

  // 根据协议和网络参数创建对应的服务端对象。
  g_server = init_server();

  // serve() 通常会阻塞在这里，直到：
  // - 收到退出信号；
  // - 启动失败；
  // - 或服务端内部主动结束。
  g_server->serve();

  LOG_INFO("Server stopped");

  // 清理全局资源，例如日志、存储、配置等模块。
  cleanup();

  // g_server 是手动 new 出来的，这里负责释放。
  delete g_server;
  return 0;
}
