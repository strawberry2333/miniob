# 公共工具库：日志、字符串、线程池、内存等基础设施
ADD_SUBDIRECTORY(common)
# 数据库服务端核心：SQL 解析、执行引擎、存储引擎等
ADD_SUBDIRECTORY(observer)
# 交互式命令行客户端
ADD_SUBDIRECTORY(obclient)
# LSM-Tree 存储引擎实现
ADD_SUBDIRECTORY(oblsm)

# C++ 语法练习模块，仅在 WITH_CPPLINGS=ON 时编译
if (WITH_CPPLINGS)
    ADD_SUBDIRECTORY(cpplings)
endif()

# 内存追踪工具，用于检测内存泄漏，仅在 WITH_MEMTRACER=ON 时编译
if (WITH_MEMTRACER)
    ADD_SUBDIRECTORY(memtracer)
endif()
