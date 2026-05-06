# 收集 oblsm 目录下所有 .cpp 源文件（递归）
file(GLOB_RECURSE OBLSM_SOURCES "*.cpp")
# 收集 CLI 工具的源文件
file(GLOB CLIUTIL_SOURCES "client/cliutil/*.cpp")
# 从库源文件中排除 client/ 和 benchmarks/ 目录，避免将可执行程序的入口编译进库
list(FILTER OBLSM_SOURCES EXCLUDE REGEX "client/.*")
list(FILTER OBLSM_SOURCES EXCLUDE REGEX "benchmarks/.*")

# 指定静态库的输出目录
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
# 将 oblsm 编译为静态库
add_library(oblsm STATIC ${OBLSM_SOURCES})
message("OBLSM_SOURCES: ${OBLSM_SOURCES}")

# 查找 jsoncpp 依赖（用于 SST 元数据的 JSON 序列化）
FIND_PACKAGE(jsoncpp CONFIG REQUIRED)

# 链接 oblsm 库的依赖：公共工具库、pthread 线程库、jsoncpp
TARGET_LINK_LIBRARIES(oblsm common pthread JsonCpp::JsonCpp)

# 编译交互式命令行客户端可执行文件
ADD_EXECUTABLE(oblsm_cli ${CLIUTIL_SOURCES} client/ob_lsm_client.cpp)
TARGET_LINK_LIBRARIES(oblsm_cli oblsm)

# 编译性能基准测试可执行文件
ADD_EXECUTABLE(oblsm_bench benchmarks/ob_lsm_bench.cpp)
TARGET_LINK_LIBRARIES(oblsm_bench oblsm)
