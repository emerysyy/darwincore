#
# DarwinCore Network 测试
#
# 功能说明：
#   DarwinCore Network 模块的集成测试（直接编译源码）
#
# 作者: DarwinCore Network 团队
# 日期: 2026

cmake_minimum_required(VERSION 3.16)
project(darwincore_network_test CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 获取父目录路径
get_filename_component(PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

# 包含头文件路径
include_directories(
    ${PARENT_DIR}/include
)

# Network 模块源文件
set(NETWORK_SOURCES
    ${PARENT_DIR}/src/darwincore/network/acceptor.cpp
    ${PARENT_DIR}/src/darwincore/network/client.cpp
    ${PARENT_DIR}/src/darwincore/network/io_monitor.cpp
    ${PARENT_DIR}/src/darwincore/network/reactor.cpp
    ${PARENT_DIR}/src/darwincore/network/server.cpp
    ${PARENT_DIR}/src/darwincore/network/socket_helper.cpp
    ${PARENT_DIR}/src/darwincore/network/worker_pool.cpp
)

# 创建测试可执行文件，直接编译 network 源码
add_executable(server_client_test
    server_client_test.cpp
    ${NETWORK_SOURCES}
)

# 编译选项
target_compile_options(server_client_test PRIVATE -g -O0)
