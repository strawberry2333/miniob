#
# observer 目标的构建脚本。
#
# 这个文件主要完成四件事：
# 1. 生成 SQL parser/lexer；
# 2. 收集 observer 除 main.cpp 之外的实现源码，构造成静态库；
# 3. 把 main.cpp 单独编成 observer 可执行文件，并链接 observer_static；
# 4. 指定输出目录和安装规则。
#

MESSAGE(STATUS "This is CMAKE_CURRENT_SOURCE_DIR dir " ${CMAKE_CURRENT_SOURCE_DIR})

# 让 observer 子目录下的头文件可以直接被当前目标引用。
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

# SQL 解析器依赖 flex/bison 在构建时生成源码。
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

# parser 相关生成文件统一落在 sql/parser 目录下，便于和手写代码放在一起编译。
set(PARSER_WORKDIR ${CMAKE_CURRENT_SOURCE_DIR}/sql/parser)
set(LEXER_SOURCES ${PARSER_WORKDIR}/lex_sql.cpp)
set(LEXER_HEADERS ${PARSER_WORKDIR}/lex_sql.h)
set(PARSER_SOURCES ${PARSER_WORKDIR}/yacc_sql.cpp)
set(PARSER_HEADERS ${PARSER_WORKDIR}/yacc_sql.hpp)

flex_target(lex ${PARSER_WORKDIR}/lex_sql.l "${LEXER_SOURCES}" DEFINES_FILE "${LEXER_HEADERS}")
bison_target(yacc ${PARSER_WORKDIR}/yacc_sql.y "${PARSER_SOURCES}" DEFINES_FILE "${PARSER_HEADERS}")
# yacc 依赖 lex 先完成生成，避免并行构建时出现顺序问题。
add_flex_bison_dependency(lex yacc)

# 扫描 observer 子目录下的所有 C/C++ 源文件，再把 main.cpp 单独摘出来作为可执行入口。
FILE(GLOB_RECURSE ALL_SRC *.cpp *.c)
SET(MAIN_SRC ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
MESSAGE("MAIN SRC: " ${MAIN_SRC})
SET(LIB_SRC ${LIB_SRC} ${LEXER_SOURCES} ${PARSER_SOURCES})
FOREACH (F ${ALL_SRC})

    IF (NOT ${F} STREQUAL ${MAIN_SRC})
        # main.cpp 只用于最终可执行程序，其余源码全部编入 observer_static。
        SET(LIB_SRC ${LIB_SRC} ${F})
        MESSAGE("Use " ${F})
    ENDIF()

ENDFOREACH (F)

SET(LIBEVENT_STATIC_LINK TRUE)
FIND_PACKAGE(Libevent CONFIG REQUIRED)
FIND_PACKAGE(jsoncpp CONFIG REQUIRED)

# observer_static 额外依赖的基础库：线程、动态库接口以及网络事件库。
SET(LIBRARIES common pthread dl libevent::core libevent::pthreads)

# 指定目标文件位置
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
MESSAGE("Binary directory:" ${EXECUTABLE_OUTPUT_PATH})
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
MESSAGE("Archive directory:" ${LIBRARY_OUTPUT_PATH})

# observer 可执行文件本身只包含 main.cpp，实际逻辑全部来自 observer_static。
ADD_EXECUTABLE(observer ${MAIN_SRC})
TARGET_LINK_LIBRARIES(observer observer_static)

# 把 observer 目录下的大部分实现编成静态库，方便测试或其它目标复用。
ADD_LIBRARY(observer_static STATIC ${LIB_SRC})

SET_TARGET_PROPERTIES(observer_static PROPERTIES OUTPUT_NAME observer)
TARGET_LINK_LIBRARIES(observer_static ${LIBRARIES} oblsm)

# Target 必须在定义 ADD_EXECUTABLE 之后， programs 不受这个限制
# TARGETS和PROGRAMS 的默认权限是OWNER_EXECUTE, GROUP_EXECUTE, 和WORLD_EXECUTE，即755权限， programs 都是处理脚本类
# 类型分为RUNTIME／LIBRARY／ARCHIVE, prog
INSTALL(TARGETS observer observer_static 
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib)
