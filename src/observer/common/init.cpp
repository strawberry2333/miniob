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
// Created by Longda on 2021/5/3.
//

#include "common/init.h"

#include "common/conf/ini.h"
#include "common/lang/string.h"
#include "common/lang/iostream.h"
#include "common/log/log.h"
#include "common/os/path.h"
#include "common/os/pidfile.h"
#include "common/os/process.h"
#include "common/os/signal.h"
#include "global_context.h"
#include "session/session.h"
#include "session/session_stage.h"
#include "sql/plan_cache/plan_cache_stage.h"
#include "storage/buffer/disk_buffer_pool.h"
#include "storage/default/default_handler.h"
#include "storage/trx/trx.h"

using namespace common;

/**
 * @brief observer 全局初始化与清理逻辑的实现文件。
 * @details 这里负责把配置、日志、全局存储对象和进程级状态按顺序装配起来，
 * 并提供与之对应的逆序清理逻辑。
 */

/**
 * @brief 返回保存“是否已初始化”标记的静态地址。
 * @return 指向初始化标记的指针引用。
 * @note 用指针而不是裸静态变量，是为了在多个辅助函数之间共享同一块可写状态。
 */
bool *&_get_init()
{
  static bool  util_init   = false;
  static bool *util_init_p = &util_init;
  return util_init_p;
}

/// @brief 查询 observer 公共初始化流程是否已经执行过。
bool get_init() { return *_get_init(); }

/// @brief 更新 observer 公共初始化标记。
void set_init(bool value) { *_get_init() = value; }

/**
 * @brief 进程信号处理占位函数。
 * @param sig 收到的信号编号。
 * @note 当前这里只记录日志，真正的退出逻辑由 main.cpp 中的 quit_signal_handle 驱动。
 */
void sig_handler(int sig)
{
  // Signal handler will be add in the next step.
  //  Add action to shutdown

  LOG_INFO("Receive one signal of %d.", sig);
}

/**
 * @brief 初始化日志系统。
 * @param process_cfg 进程启动参数。
 * @param properties 已加载的配置集合。
 * @return 0 表示成功，非 0 表示失败。
 * @note 该函数会读取 [LOG] 配置段，决定日志文件名、文件级别和控制台级别。
 */
int init_log(ProcessParam *process_cfg, Ini &properties)
{
  const string &proc_name = process_cfg->get_process_name();
  try {
    // we had better alloc one lock to do so, but simplify the logic
    if (g_log) {
      return 0;
    }

    // 把当前 Session 地址挂到日志上下文里，便于排查请求级别的问题。
    auto log_context_getter = []() { return reinterpret_cast<intptr_t>(Session::current_session()); };

    const string        log_section_name = "LOG";
    map<string, string> log_section      = properties.get(log_section_name);

    string log_file_name;

    // 先决定日志文件名：配置里没写就退回到 <process_name>.log。
    string key = "LOG_FILE_NAME";

    map<string, string>::iterator it = log_section.find(key);
    if (it == log_section.end()) {
      log_file_name = proc_name + ".log";
      cout << "Not set log file name, use default " << log_file_name << endl;
    } else {
      log_file_name = it->second;
    }

    log_file_name = getAboslutPath(log_file_name.c_str());

    // 文件日志级别与控制台日志级别分开读取，方便分别控制磁盘与终端噪音。
    LOG_LEVEL log_level = LOG_LEVEL_INFO;
    key                 = ("LOG_FILE_LEVEL");
    it                  = log_section.find(key);
    if (it != log_section.end()) {
      int log = (int)log_level;
      str_to_val(it->second, log);
      log_level = (LOG_LEVEL)log;
    }

    LOG_LEVEL console_level = LOG_LEVEL_INFO;
    key                     = ("LOG_CONSOLE_LEVEL");
    it                      = log_section.find(key);
    if (it != log_section.end()) {
      int log = (int)console_level;
      str_to_val(it->second, log);
      console_level = (LOG_LEVEL)log;
    }

    // 真正创建默认 logger，并注册上下文 getter。
    LoggerFactory::init_default(log_file_name, log_level, console_level);
    g_log->set_context_getter(log_context_getter);

    key = ("DefaultLogModules");
    it  = log_section.find(key);
    if (it != log_section.end()) {
      g_log->set_default_module(it->second);
    }

    if (process_cfg->is_demon()) {
      // 守护进程模式下，把 stdout/stderr 也重定向到日志文件，避免输出丢失。
      sys_log_redirect(log_file_name.c_str(), log_file_name.c_str());
    }

    return 0;
  } catch (exception &e) {
    cerr << "Failed to init log for " << proc_name << SYS_OUTPUT_FILE_POS << SYS_OUTPUT_ERROR << endl;
    return errno;
  }

  return 0;
}

/**
 * @brief 销毁全局日志对象。
 * @details cleanup 阶段调用，用于释放 init_log 创建的默认 logger。
 */
void cleanup_log()
{
  if (g_log) {
    delete g_log;
    g_log = nullptr;
  }
}

/**
 * @brief 为 SEDA 初始化预留的钩子。
 * @return 当前总是返回成功。
 */
int prepare_init_seda()
{
  return 0;
}

/**
 * @brief 初始化 observer 级全局对象。
 * @param process_param 进程启动参数。
 * @param properties 已加载配置。
 * @return 0 表示成功，非 0 表示失败。
 * @note 当前最核心的全局对象是 DefaultHandler，它负责存储层的初始化。
 */
int init_global_objects(ProcessParam *process_param, Ini &properties)
{
  GCTX.handler_ = new DefaultHandler();

  int ret = 0;

  // 存储引擎、事务组件和 durability mode 都通过启动参数透传给 DefaultHandler。
  RC rc = GCTX.handler_->init("miniob", 
                              process_param->trx_kit_name().c_str(),
                              process_param->durability_mode().c_str(),
                              process_param->storage_engine().c_str());
  if (OB_FAIL(rc)) {
    LOG_ERROR("failed to init handler. rc=%s", strrc(rc));
    return -1;
  }
  return ret;
}

/**
 * @brief 销毁 observer 级全局对象。
 * @return 0 表示成功。
 */
int uninit_global_objects()
{
  delete GCTX.handler_;
  GCTX.handler_ = nullptr;

  return 0;
}

/**
 * @brief 执行 observer 公共初始化流程。
 * @param process_param 进程启动参数。
 * @return STATUS_SUCCESS 表示成功，否则返回失败码。
 * @details 初始化顺序依次为：守护进程化、写 pid、加载配置、初始化日志、初始化全局对象。
 */
int init(ProcessParam *process_param)
{
  if (get_init()) {
    // 避免重复初始化同一套全局资源。
    return 0;
  }

  set_init(true);

  // 如果要求以守护进程方式运行，需要在尽早的阶段完成进程分叉和标准流重定向。
  int rc = STATUS_SUCCESS;
  if (process_param->is_demon()) {
    rc = daemonize_service(process_param->get_std_out().c_str(), process_param->get_std_err().c_str());
    if (rc != 0) {
      cerr << "Shutdown due to failed to daemon current process!" << endl;
      return rc;
    }
  }

  writePidFile(process_param->get_process_name().c_str());

  // 在进入多线程模式之前先初始化全局资源，避免后续出现并发竞态。
  // 先加载配置文件，后面的日志和存储初始化都依赖它。
  rc = get_properties()->load(process_param->get_conf());
  if (rc) {
    cerr << "Failed to load configuration files" << endl;
    return rc;
  }

  // 再初始化日志，确保后续步骤发生错误时已经具备统一日志能力。
  rc = init_log(process_param, *get_properties());
  if (rc) {
    cerr << "Failed to init Log" << endl;
    return rc;
  }

  string conf_data;
  get_properties()->to_string(conf_data);
  LOG_INFO("Output configuration \n%s", conf_data.c_str());

  // 最后初始化真正的存储/事务等全局对象。
  rc = init_global_objects(process_param, *get_properties());
  if (rc != 0) {
    LOG_ERROR("failed to init global objects");
    return rc;
  }

  // 这里预留了阻塞信号并交给专门线程处理的方案，目前仍保持注释状态。
  // setSignalHandler(sig_handler);
  // sigset_t newSigset, oset;
  // blockDefaultSignals(&newSigset, &oset);
  //  wait interrupt signals
  // startWaitForSignals(&newSigset);

  LOG_INFO("Successfully init utility");

  return STATUS_SUCCESS;
}

/**
 * @brief 执行 observer 公共清理流程。
 * @details 释放顺序大体与初始化顺序相反：先销毁全局对象，再释放配置，最后关闭日志。
 */
void cleanup_util()
{
  uninit_global_objects();

  if (nullptr != get_properties()) {
    delete get_properties();
    get_properties() = nullptr;
  }

  LOG_INFO("Shutdown Cleanly!");

  // Finalize tracer
  cleanup_log();

  set_init(false);
}

/// @brief 对外暴露的 cleanup 包装器。
void cleanup() { cleanup_util(); }
