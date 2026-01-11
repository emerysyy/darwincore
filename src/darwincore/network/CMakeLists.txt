# ========================================
# Network 网络库构建配置
# ========================================
# 功能：编译 network 静态和动态库
# 包含模块：acceptor, reactor, worker_pool, server, client
#
# 对外暴露的头文件（在 include/darwincore/network/）：
#   - client.h: 客户端接口
#   - configuration.h: Socket 配置
#   - event.h: 事件定义
#   - server.h: 服务器接口
#
# 内部头文件（在 src/darwincore/network/）：
#   - acceptor.h: 接收器实现
#   - concurrent_queue.h: 线程安全队列
#   - io_monitor.h: IO 监控器封装
#   - reactor.h: Reactor 实现
#   - reactor_connection.h: Reactor 内部连接结构
#   - socket_helper.h: Socket 辅助函数
#   - worker_pool.h: 工作线程池
# ========================================

# 设置目标库名称
SET(LOCAL_TARGET darwincore_network)

# 查找所有源文件
file(GLOB LOCAL_SOURCES *.cpp *.c)

# 创建动态库
add_library(${LOCAL_TARGET} SHARED ${LOCAL_SOURCES})
target_compile_definitions(${LOCAL_TARGET} PRIVATE DARWINCORE_NETWORK_BUILD)

# 添加头文件搜索路径
# 内部头文件与源文件在同一目录
target_include_directories(${LOCAL_TARGET} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
# 对外头文件在 include 目录
target_include_directories(${LOCAL_TARGET} PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

# 创建静态库
add_library(${LOCAL_TARGET}_static STATIC ${LOCAL_SOURCES})
target_compile_definitions(${LOCAL_TARGET}_static PRIVATE DARWINCORE_NETWORK_STATIC)
SET_TARGET_PROPERTIES(${LOCAL_TARGET}_static PROPERTIES OUTPUT_NAME "${LOCAL_TARGET}")

# 添加头文件搜索路径
# 内部头文件与源文件在同一目录
target_include_directories(${LOCAL_TARGET}_static PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
# 对外头文件在 include 目录
target_include_directories(${LOCAL_TARGET}_static PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

# ========================================
# 安装配置
# ========================================
# 只安装对外暴露的头文件
# ========================================
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/darwincore/network/
        DESTINATION ${PROJECT_SOURCE_DIR}/build/export/include/darwincore/network
        FILES_MATCHING PATTERN "*.h")
INSTALL(TARGETS ${LOCAL_TARGET} DESTINATION ${PROJECT_SOURCE_DIR}/build/export/lib)
INSTALL(TARGETS ${LOCAL_TARGET}_static DESTINATION ${PROJECT_SOURCE_DIR}/build/export/lib)
