# DarwinCore - 现代 C++ 基础库

## 项目简介

DarwinCore 是一个现代化的 C++ 基础库，提供了网络通信、文件操作、日志系统、线程管理等常用功能，旨在简化 C++ 项目开发，提高代码质量和开发效率。

### 主要特性

- 🚀 **高性能**：基于 C++17 标准，充分利用现代 C++ 特性
- 🧩 **模块化设计**：清晰的模块划分，按需引用
- 📦 **易于集成**：统一的头文件路径 `#include <darwincore/xxx>`
- 🔧 **跨平台**：支持 macOS、Linux 等主流平台
- 📝 **完善文档**：详细的代码注释和使用示例
- 🛡️ **类型安全**：强类型设计，编译时错误检查
- ⚡ **异步支持**：提供异步日志、异步队列等高性能组件

---

## 目录

- [快速开始](#快速开始)
- [项目结构](#项目结构)
- [模块说明](#模块说明)
- [构建指南](#构建指南)
- [使用示例](#使用示例)
- [API 文档](#api-文档)
- [开发规范](#开发规范)
- [版本历史](#版本历史)
- [贡献指南](#贡献指南)

---

## 快速开始

### 安装

```bash
# 克隆仓库
git clone https://github.com/yourname/DarwinCore.git
cd DarwinCore

# 构建
./build.sh
```

### 使用示例

```cpp
#include <darwincore/network/SocketServer.h>
#include <darwincore/foundation/logger/Logger.h>

using namespace darwincore;
using namespace darwincore::network;
using namespace darwincore::log;

int main() {
    // 初始化日志系统
    LOG_INFO("DarwinCore Example Started");

    // 创建 TCP 服务器
    TCPSocketServer server;
    if (!server.startIPSocketServer("127.0.0.1", 8080)) {
        LOG_ERROR("Failed to start server");
        return -1;
    }

    LOG_INFO("Server started on port 8080");
    return 0;
}
```

---

## 项目结构

```
DarwinCore/
├── CMakeLists.txt              # 主 CMake 配置文件
├── build.sh                    # 自动化构建脚本
├── README.md                   # 项目说明文档
│
├── include/                    # 头文件目录（对外接口）
│   └── darwincore/
│       ├── foundation/          # Foundation 基础库
│       │   ├── algorithm/      # 算法模块（布隆过滤器、哈希、搜索、排序）
│       │   ├── command/        # Shell 命令执行器
│       │   ├── common/         # 通用工具函数
│       │   ├── container/      # 容器（位集、循环缓冲区、LRU缓存等）
│       │   ├── date/           # 日期时间处理（DateTime、Duration、Timestamp等）
│       │   ├── file/           # 文件操作（文件管理、路径、锁、监视器等）
│       │   ├── logger/         # 日志系统（多级别、多输出、异步日志）
│       │   ├── memory/         # 内存管理（内存分配器）
│       │   ├── process/        # 进程管理（进程树、共享内存、工具类等）
│       │   ├── sqlite3/        # SQLite3 数据库封装
│       │   ├── string/         # 字符串处理（编码、格式化、正则、工具类等）
│       │   ├── thread/         # 线程同步（异步队列、锁、运行循环等）
│       │   ├── throttle/       # 限流器
│       │   ├── timer/          # 定时器
│       │   └── uuid/          # UUID 生成器
│       ├── network/            # 网络通信模块
│       │   ├── SocketServer.h  # TCP 服务器
│       │   ├── SocketClient.h  # TCP 客户端
│       │   └── SocketUtil.h   # 网络工具
│       └── asar/              # ASAR 模块（待实现）
│
└── src/                       # 源代码目录（实现文件）
    └── darwincore/
        ├── CMakeLists.txt      # 库模块构建配置
        ├── foundation/         # Foundation 实现（仅 .cpp 文件）
        ├── network/           # Network 实现（仅 .cpp 文件）
        └── asar/             # ASAR 实现（待实现）
```

---

## 模块说明

### Foundation 模块

Foundation 模块提供了应用开发所需的基础功能。

#### 📊 Algorithm - 算法模块

- **BloomFilter**: 布隆过滤器，用于快速判断元素是否在集合中
- **Hash**: 常用哈希函数（MD5、SHA1、SHA256、FNV 等）
- **Search**: 搜索算法（二分查找、KMP 等）
- **Sort**: 排序算法（快速排序、归并排序等）

```cpp
#include <darwincore/foundation/algorithm/BloomFilter.h>
#include <darwincore/foundation/algorithm/Hash.h>

BloomFilter filter(1000);
filter.add("hello");
if (filter.contains("hello")) {
    std::cout << "Found!" << std::endl;
}

std::string hash = Hash::md5("hello");
```

#### 💻 Command - 命令执行

- **ShellExecutor**: Shell 命令执行器，支持同步和异步执行

```cpp
#include <darwincore/foundation/command/ShellExecutor.h>

ShellExecutor executor;
std::string output = executor.execute("ls -la");
```

#### 🛠️ Common - 通用工具

- **Common**: 提供线程休眠、工具函数等

```cpp
#include <darwincore/foundation/common/Common.h>

Common::sleep_ms(100);  // 休眠 100 毫秒
```

#### 📦 Container - 容器

- **BitSet**: 位集，用于高效的位操作
- **CircularBuffer**: 循环缓冲区，适用于流数据
- **ConcurrentMap**: 线程安全的映射容器
- **LRUCache**: 最近最少使用缓存
- **ObjectPool**: 对象池，减少内存分配
- **RingQueue**: 环形队列

```cpp
#include <darwincore/foundation/container/LRUCache.h>

LRUCache<std::string, int> cache(100);
cache.put("key1", 1);
int value = cache.get("key1");
```

#### 📅 Date - 日期时间

- **DateTime**: 日期时间类，支持格式化、时区转换、时间戳操作
- **Duration**: 时间段类，用于计算时间差
- **Timestamp**: 时间戳类，高精度时间记录
- **Calendar**: 日历类，日期计算
- **TimeZone**: 时区类，时区转换

```cpp
#include <darwincore/foundation/date/DateTime.h>

DateTime now = DateTime::now();
std::string formatted = now.format("%Y-%m-%d %H:%M:%S");
DateTime tomorrow = now.addDays(1);
```

#### 📁 File - 文件操作

- **FileManager**: 文件和目录管理器（创建、删除、移动、复制等）
- **FilePath**: 文件路径解析和操作
- **FileHandle**: 文件读写操作
- **FileLock**: 进程间文件锁
- **FileWatcher**: 文件监视器（检测文件变化）
- **MMap**: 内存映射文件
- **PathUtils**: 路径工具类
- **TemporaryFile**: 临时文件
- **DirectoryIterator**: 目录遍历器
- **SecureDelete**: 安全删除文件

```cpp
#include <darwincore/foundation/file/FileManager.h>
#include <darwincore/foundation/file/FilePath.h>

FileManager fm;
fm.createDirectoryAtPath("/tmp/test");
fm.copyItem("/tmp/test/file.txt", "/tmp/test/file_copy.txt");

FilePath path("/tmp/test/file.txt");
std::string filename = path.filename();
```

#### 📝 Logger - 日志系统

- **Logger**: 多级别日志系统（TRACE、DEBUG、INFO、WARNING、ERROR、FATAL）
- **AsyncLogger**: 异步日志器，不阻塞主线程
- **LogFormatter**: 日志格式化器
- **LogRotate**: 日志轮转器（按大小、时间）
- **LogSink**: 日志输出抽象
  - **ConsoleSink**: 控制台输出
  - **FileSink**: 文件输出
  - **OSLogSink**: macOS 系统日志输出
  - **MultiSink**: 多输出组合
  - **CallbackSink**: 回调输出

```cpp
#include <darwincore/foundation/logger/Logger.h>

LOG_TRACE("Detailed debug information");
LOG_DEBUG("Debug message: %d", value);
LOG_INFO("Application started");
LOG_WARN("Warning: resource usage high");
LOG_ERROR("Error occurred: %s", error_msg);
LOG_FATAL("Fatal error, exiting");
```

#### 💾 Memory - 内存管理

- **MemoryAllocator**: STL 兼容的内存分配器

```cpp
#include <darwincore/foundation/memory/MemoryAllocator.h>

std::vector<int, MemoryAllocator<int>> vec;
```

#### 🔄 Process - 进程管理

- **ProcessTree**: 进程树管理，支持进程血缘关系查询
- **ProcessUtil**: 进程工具类（查询进程状态、资源使用等）
- **SharedMemory**: 进程间共享内存
- **Semaphore**: 信号量（进程间同步）

```cpp
#include <darwincore/foundation/process/ProcessUtil.h>

ProcessUtil util;
auto pids = util.getChildProcesses(1234);
```

#### 🗄️ SQLite3 - 数据库

- **SQLite3DB**: SQLite3 数据库封装
- **SQLite3DB::Record**: 查询记录封装

```cpp
#include <darwincore/foundation/sqlite3/SQLite3DB.h>

SQLite3DB db;
db.open("/tmp/test.db");
db.execute("CREATE TABLE test (id INTEGER, name TEXT)");
```

#### 🔤 String - 字符串处理

- **StringUtils**: 字符串工具类（分割、连接、大小写转换、格式化等）
- **StringBuilder**: 字符串构建器，高效拼接
- **StringPool**: 字符串池，减少重复分配
- **Encoding**: 字符编码转换（UTF-8、GBK 等）
- **Regex**: 正则表达式封装
- **Format**: 格式化工具

```cpp
#include <darwincore/foundation/string/StringUtils.h>

std::vector<std::string> parts = StringUtils::split("a,b,c", ",");
std::string upper = StringUtils::toUpper("hello");
```

#### 🧵 Thread - 线程同步

- **AsyncQueue**: 异步队列（线程池）
- **Dispatch**: 任务分发（异步/同步执行）
- **Lock**: 线程锁（互斥锁、读写锁）
- **RunLoop**: 运行循环（可阻塞的线程同步机制）

```cpp
#include <darwincore/foundation/thread/AsyncQueue.h>
#include <darwincore/foundation/thread/Dispatch.h>

AsyncQueue::queue().submit([]() {
    // 异步执行的任务
});

Dispatch::main().async([]() {
    // 在主线程执行
});
```

#### 🎚️ Throttle - 限流器

- **Throttle**: 事件限流器，控制事件触发频率

```cpp
#include <darwincore/foundation/throttle/Throttle.h>

Throttle throttle(std::chrono::milliseconds(100));
if (throttle.tryFire()) {
    // 每 100 毫秒最多执行一次
}
```

#### ⏱️ Timer - 定时器

- **Timer**: 定时器（单次/重复执行）

```cpp
#include <darwincore/foundation/timer/Timer.h>

Timer timer;
timer.start(std::chrono::seconds(1), []() {
    // 每秒执行一次
}, true);  // true = 重复执行
```

#### 🆔 UUID - UUID 生成器

- **UUID**: UUID 生成器

```cpp
#include <darwincore/foundation/uuid/UUID.h>

std::string uuid = UUID::generate();
```

### Network 模块

网络模块提供了高性能的网络通信功能。

#### 🔌 Network - 网络通信

- **TCPSocketServer**: TCP 套接字服务器
  - 支持 IPv4 和 Unix 域套接字
  - 使用 kqueue 实现高效的 IO 多路复用
  - 多线程架构（监听线程 + 接收线程）
  - 支持多个客户端并发连接

- **TCPSocketClient**: TCP 套接字客户端
  - 支持 IPv4 和 Unix 域套接字
  - 单线程异步通信模型
  - 支持自动重连

- **SocketUtil**: 网络工具类
  - 日志宏（LOG_ERROR、LOG_WARN、LOG_INFO 等）
  - IO 监控器（基于 kqueue）

- **SocketConnection**: 套接字连接信息封装

```cpp
#include <darwincore/network/SocketServer.h>
#include <darwincore/network/SocketClient.h>

// TCP 服务器
TCPSocketServer server;
server.startIPSocketServer("127.0.0.1", 8080);

// TCP 客户端
TCPSocketClient client;
client.startIPClient("127.0.0.1", 8080);

// Unix 域套接字
server.startUnixDomainSocketServer("/tmp/ipc.socket");
client.startUnixDomainClient("/tmp/ipc.socket");
```

---

## 构建指南

### 前置要求

- **CMake**: 3.20 或更高版本
- **编译器**: 支持 C++17 的编译器
  - Clang 5.0+ (macOS/Linux)
  - GCC 7.0+ (Linux)
  - MSVC 2017+ (Windows，暂不支持)
- **macOS**: Xcode 命令行工具

### 构建步骤

#### Release 模式（默认）

```bash
./build.sh
```

这将：
1. 清理并重新创建 `build` 目录
2. 使用 CMake 配置项目
3. 编译生成支持 x86_64 和 arm64 双架构的库文件
4. 安装到 `build/export` 目录

#### Debug 模式（生成 Xcode 工程）

```bash
./build.sh dev
```

这将生成 Xcode 工程文件，便于使用 Xcode 进行调试。

### 手动构建

```bash
mkdir build && cd build
cmake ..
make -j4
make install
```

### 构建产物

构建完成后，以下产物将被生成到 `build/export` 目录：

```
build/export/
├── lib/
│   ├── libdarwincore_foundation.dylib    # Foundation 动态库
│   ├── libdarwincore_foundation.a       # Foundation 静态库
│   ├── libdarwincore_network.dylib      # Network 动态库
│   └── libdarwincore_network.a         # Network 静态库
└── include/
    └── darwincore/
        ├── foundation/                 # Foundation 头文件
        └── network/                   # Network 头文件
```

---

## 使用示例

### 日志系统

```cpp
#include <darwincore/foundation/logger/Logger.h>

using namespace darwincore::log;

int main() {
    // 使用日志宏
    LOG_TRACE("Trace message: value=%d", 42);
    LOG_DEBUG("Debug message");
    LOG_INFO("Info message");
    LOG_WARN("Warning message");
    LOG_ERROR("Error message: %s", "file not found");
    LOG_FATAL("Fatal error");

    return 0;
}
```

### 文件操作

```cpp
#include <darwincore/foundation/file/FileManager.h>
#include <darwincore/foundation/file/FilePath.h>

using namespace darwincore::file;

int main() {
    FileManager fm;

    // 创建目录
    fm.createDirectoryAtPath("/tmp/test");

    // 复制文件
    fm.copyItem("/tmp/source.txt", "/tmp/dest.txt");

    // 路径操作
    FilePath path("/tmp/test/file.txt");
    std::string filename = path.filename();
    std::string parent = path.parentPath();

    return 0;
}
```

### TCP 服务器

```cpp
#include <darwincore/network/SocketServer.h>

using namespace darwincore::network;

class MyServerDelegate : public TCPSocketServerDelegate {
public:
    void onConnectionChanged(const TCPSocketServer *server,
                           SocketConnection connection,
                           bool connected) override {
        if (connected) {
            LOG_INFO("Client connected: %s:%d",
                     connection.host.c_str(), connection.port);
        } else {
            LOG_INFO("Client disconnected");
        }
    }

    void onReceive(const TCPSocketServer *server,
                  SocketConnection connection,
                  uint8 *data, int size) override {
        std::string msg((char*)data, size);
        LOG_INFO("Received: %s", msg.c_str());
    }
};

int main() {
    TCPSocketServer server;
    server.setDelegate(std::make_shared<MyServerDelegate>());

    if (!server.startIPSocketServer("127.0.0.1", 8080)) {
        LOG_ERROR("Failed to start server");
        return -1;
    }

    LOG_INFO("Server started, press Ctrl+C to stop");
    getchar();

    server.stopServer();
    return 0;
}
```

### TCP 客户端

```cpp
#include <darwincore/network/SocketClient.h>

using namespace darwincore::network;

class MyClientDelegate : public TCPSocketClientDelegate {
public:
    void onConnectionChanged(const TCPSocketClient *client,
                           bool connected) override {
        if (connected) {
            LOG_INFO("Connected to server");
        } else {
            LOG_INFO("Disconnected from server");
        }
    }

    void onConnectionError(const TCPSocketClient *client,
                          const char *error) override {
        LOG_ERROR("Connection error: %s", error);
    }

    void onReceive(const TCPSocketClient *client,
                  const uint8 *buf, int size) override {
        std::string msg((char*)buf, size);
        LOG_INFO("Received: %s", msg.c_str());
    }
};

int main() {
    TCPSocketClient client;
    client.setDelegate(std::make_shared<MyClientDelegate>());

    if (!client.startIPClient("127.0.0.1", 8080)) {
        LOG_ERROR("Failed to connect");
        return -1;
    }

    // 发送数据
    std::string msg = "Hello, Server!";
    client.sendData((uint8*)msg.c_str(), msg.size());

    getchar();
    client.stopClient();
    return 0;
}
```

### 日期时间操作

```cpp
#include <darwincore/foundation/date/DateTime.h>
#include <darwincore/foundation/date/Duration.h>

using namespace darwincore::date;

int main() {
    // 获取当前时间
    DateTime now = DateTime::now();

    // 格式化输出
    std::string formatted = now.format("%Y-%m-%d %H:%M:%S");
    std::cout << "Current time: " << formatted << std::endl;

    // 时间运算
    DateTime tomorrow = now.addDays(1);
    DateTime nextHour = now.addHours(1);

    // 时间差
    DateTime birth = DateTime::fromTimestamp(1609459200); // 2021-01-01
    double days = now.diffDays(birth);

    // 解析字符串
    auto parsed = DateTime::parse("2024-01-01 12:00:00");
    if (parsed.has_value()) {
        std::cout << "Parsed: " << parsed->format() << std::endl;
    }

    return 0;
}
```

### 异步任务队列

```cpp
#include <darwincore/foundation/thread/AsyncQueue.h>

using namespace darwincore::thread;

int main() {
    // 提交异步任务
    AsyncQueue::queue().submit([]() {
        LOG_INFO("Async task 1");
    });

    AsyncQueue::queue().submit([]() {
        LOG_INFO("Async task 2");
    });

    // 等待所有任务完成
    std::this_thread::sleep_for(std::chrono::seconds(1));

    return 0;
}
```

---

## API 文档

详细的 API 文档请参考各模块的头文件注释。

### 命名规范

- **命名空间**: `darwincore`
- **子命名空间**:
  - `darwincore::network` - 网络模块
  - `darwincore::log` - 日志模块
  - `darwincore::file` - 文件模块
  - `darwincore::date` - 日期时间模块
  - `darwincore::string` - 字符串模块
  - `darwincore::thread` - 线程模块
  - 等

### 头文件引用

所有头文件使用统一的引用路径：

```cpp
#include <darwincore/network/SocketServer.h>
#include <darwincore/foundation/logger/Logger.h>
#include <darwincore/foundation/date/DateTime.h>
#include <darwincore/foundation/file/FileManager.h>
```

---

## 开发规范

### 命名规范

| 类型 | 规范 | 示例 |
|------|------|------|
| 类名 | 大驼峰 (PascalCase) | `TCPSocketServer`, `DateTime` |
| 方法名 | 小驼峰 (camelCase) | `startIPServer`, `format` |
| 私有成员变量 | 下划线前缀 | `_server_fd`, `_running` |
| 常量 | 全大写下划线分隔 | `SOCKET_TYPE_IPV4`, `MAX_SIZE` |
| 宏定义 | 全大写下划线分隔 | `LOG_INFO`, `MAX_COUNT` |
| 文件名 | 大驼峰 (PascalCase) | `SocketServer.h`, `DateTime.cpp` |

### 代码风格

- **缩进**: 使用 4 空格，不使用 Tab
- **行宽**: 每行不超过 120 字符
- **头文件保护**: 使用 `#ifndef` 宏保护
  ```cpp
  #ifndef DARWINCORE_SOCKET_SERVER_H
  #define DARWINCORE_SOCKET_SERVER_H
  // ...
  #endif // DARWINCORE_SOCKET_SERVER_H
  ```
- **命名空间**: 使用 `darwincore` 作为顶层命名空间

### 注释规范

#### 文件头注释

```cpp
//
// File.h
// DarwinCore
//
// 功能描述：简要说明文件的功能
//
```

#### 类注释

```cpp
/**
 * @brief 类的简要描述
 *
 * 详细描述类的功能、使用方法和注意事项
 */
class MyClass {
    // ...
};
```

#### 方法注释

```cpp
/**
 * @brief 方法简要描述
 * @param param1 参数1说明
 * @param param2 参数2说明
 * @return 返回值说明
 */
int myMethod(int param1, std::string param2) {
    // ...
}
```

---

## 版本历史

### v1.0.0 (2026-01-07)

#### 新特性

- ✨ 完成项目重构，头文件和实现文件分离
- ✨ 统一的头文件路径：`#include <darwincore/xxx>`
- ✨ 实现 Foundation 基础库（14 个子模块）
- ✨ 实现 Network 网络库
- ✨ 支持双架构编译（x86_64 和 arm64）
- ✨ 提供完整的日志系统（异步、多输出、轮转）
- ✨ 提供高性能网络通信（kqueue、多路复用）

#### 模块列表

- Algorithm（算法模块）
  - BloomFilter（布隆过滤器）
  - Hash（哈希函数）
  - Search（搜索算法）
  - Sort（排序算法）

- Command（命令执行）
  - ShellExecutor（Shell 执行器）

- Common（通用工具）
  - Common（工具函数）

- Container（容器）
  - BitSet（位集）
  - CircularBuffer（循环缓冲区）
  - ConcurrentMap（并发映射）
  - LRUCache（LRU 缓存）
  - ObjectPool（对象池）
  - RingQueue（环形队列）

- Date（日期时间）
  - Calendar（日历）
  - DateTime（日期时间）
  - Duration（时间段）
  - Timestamp（时间戳）
  - TimeZone（时区）

- File（文件操作）
  - DirectoryIterator（目录遍历）
  - FileHandle（文件句柄）
  - FileLock（文件锁）
  - FileManager（文件管理器）
  - FilePath（文件路径）
  - FileWatcher（文件监视器）
  - MMap（内存映射）
  - PathUtils（路径工具）
  - SecureDelete（安全删除）
  - TemporaryFile（临时文件）

- Logger（日志系统）
  - AsyncLogger（异步日志器）
  - LogFormatter（日志格式化器）
  - LogRotate（日志轮转器）
  - Logger（日志器）
  - LogSink（日志输出）
    - ConsoleSink（控制台输出）
    - FileSink（文件输出）
    - OSLogSink（系统日志输出）
    - MultiSink（多输出组合）
    - CallbackSink（回调输出）

- Memory（内存管理）
  - MemoryAllocator（内存分配器）

- Process（进程管理）
  - ProcessTree（进程树）
  - ProcessUtil（进程工具）
  - SharedMemory（共享内存）
  - Singleton（单例模式）
  - Semaphore（信号量）

- SQLite3（数据库）
  - SQLite3DB（SQLite3 封装）

- String（字符串处理）
  - Encoding（编码转换）
  - Format（格式化工具）
  - Regex（正则表达式）
  - StringBuilder（字符串构建器）
  - StringPool（字符串池）
  - StringUtils（字符串工具）

- Thread（线程同步）
  - AsyncQueue（异步队列）
  - Dispatch（任务分发）
  - Lock（锁）
  - RunLoop（运行循环）

- Throttle（限流器）
  - Throttle（事件限流器）

- Timer（定时器）
  - Timer（定时器）

- UUID（UUID 生成器）
  - UUID（UUID 生成器）

- Network（网络通信）
  - SocketClient（TCP 客户端）
  - SocketServer（TCP 服务器）
  - SocketUtil（网络工具）

---

## 贡献指南

欢迎贡献代码、报告问题或提出建议！

### 提交 Issue

- 在 GitHub 上提交 Issue 时，请提供详细的问题描述
- 包含复现步骤、期望行为和实际行为
- 如果是 Bug，请提供环境信息（操作系统、编译器版本等）

### 提交代码

1. Fork 本仓库
2. 创建特性分支（`git checkout -b feature/AmazingFeature`）
3. 提交更改（`git commit -m 'Add some AmazingFeature'`）
4. 推送到分支（`git push origin feature/AmazingFeature`）
5. 提交 Pull Request

### 代码审查

- 确保代码符合项目的代码风格
- 添加必要的测试和文档
- 确保所有测试通过

---

## 许可证

本项目采用 MIT 许可证。详见 [LICENSE](LICENSE) 文件。

---

## 联系方式

- **作者**: Darwin Core
- **项目主页**: [GitHub](https://github.com/yourname/DarwinCore)
- **问题反馈**: [Issues](https://github.com/yourname/DarwinCore/issues)

---

<div align="center">

**感谢使用 DarwinCore！**

如果这个项目对你有帮助，请给它一个 ⭐️

</div>
