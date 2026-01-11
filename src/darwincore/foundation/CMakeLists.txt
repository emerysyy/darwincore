# ========================================
# Foundation 基础库构建配置
# ========================================
# 功能：编译 foundation 静态和动态库
# 包含模块：日志、文件、线程、字符串、定时器、进程、数据库等
# ========================================

# 设置目标库名称
SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(LOCAL_TARGET darwincore_foundation)

# 使用 GLOB_RECURSE 递归查找所有源文件和头文件
# 注释掉的代码使用非递归方式查找，不包含子目录
#file(GLOB LOCAL_SOURCES */*.cpp */*.c)
#file(GLOB LOCAL_HEADERS */*.hpp */*.h)

file(GLOB_RECURSE LOCAL_SOURCES *.cpp *.c)
# 头文件现在从 include 目录获取，不包含在源文件列表中
# file(GLOB_RECURSE LOCAL_HEADERS *.hpp *.h)

# 创建动态库（Shared Library）
add_library(${LOCAL_TARGET} SHARED ${LOCAL_SOURCES})

# 添加头文件搜索路径
target_include_directories(${LOCAL_TARGET} PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

# 动态库版本设置（可选）
# VERSION: 库的完整版本号（如 1.0）
# SOVERSION: API 版本号（如 1.0.1），用于 ABI 兼容性检查
#SET_TARGET_PROPERTIES(${LOCAL_TARGET} PROPERTIES VERSION 1.0 SOVERSION 1.0.1)
#SET_TARGET_PROPERTIES(${LOCAL_TARGET} PROPERTIES VERSION 1.2)
#SET_TARGET_PROPERTIES(${LOCAL_TARGET} PROPERTIES SOVERSION 1.0.1)


# 调试输出（已注释）
#MESSAGE(FATAL_ERROR "Build Error ${PROJECT_BINARY_DIR}")
#MESSAGE(STATUS "Project Binary Dir ${PROJECT_BINARY_DIR}")
#MESSAGE("Project Source Dir ${PROJECT_SOURCE_DIR}")

# 创建静态库（Static Library）
# 使用 SET_TARGET_PROPERTIES 将静态库的输出名称设置为与动态库相同
add_library(${LOCAL_TARGET}_static STATIC ${LOCAL_SOURCES})
SET_TARGET_PROPERTIES(${LOCAL_TARGET}_static PROPERTIES OUTPUT_NAME "${LOCAL_TARGET}")

# 添加头文件搜索路径
target_include_directories(${LOCAL_TARGET}_static PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

if(APPLE)
    target_link_libraries(${LOCAL_TARGET} PRIVATE "-framework CoreFoundation")
    target_link_libraries(${LOCAL_TARGET} PRIVATE "-framework CoreServices")
    target_link_libraries(${LOCAL_TARGET}_static INTERFACE "-framework CoreFoundation")
    target_link_libraries(${LOCAL_TARGET}_static INTERFACE "-framework CoreServices")
endif()

# ========================================
# 安装配置
# ========================================
# 将头文件从 include 目录安装到安装目录
# 将库文件安装到 lib 目录
# ========================================
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/darwincore/foundation/
        DESTINATION ${PROJECT_SOURCE_DIR}/build/export/include/darwincore/foundation
        FILES_MATCHING PATTERN "*.h")
INSTALL(TARGETS ${LOCAL_TARGET} DESTINATION ${PROJECT_SOURCE_DIR}/build/export/lib)
INSTALL(TARGETS ${LOCAL_TARGET}_static DESTINATION ${PROJECT_SOURCE_DIR}/build/export/lib)
