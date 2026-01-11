# CLAUDE.md

此文件为 Claude Code (claude.ai/code) 在此代码库中工作时提供指导。

## 构建命令

### Release 构建（默认）
```bash
./build.sh
```
构建双架构（x86_64 + arm64）的动态/静态库，包含 Debug 符号。输出到 `build/export/`。

### Debug 构建（生成 Xcode 工程）
```bash
./build.sh dev
```
生成 Xcode 工程文件，位于 `build/DarwinCore.xcodeproj`，便于使用 Xcode 调试。

### 手动构建
```bash
mkdir build && cd build
cmake ..
make -j4
make install
```

### 构建产物
构建完成后，库文件安装到 `build/export/`：
- `lib/libdarwincore_network.dylib` - Network 动态库
- `lib/libdarwincore_network.a` - Network 静态库
- `include/darwincore/network/` - 公共头文件

## 运行测试

测试代码位于 `test/` 目录，可以使用 CMake 构建：
```bash
cd test
mkdir build && cd build
cmake ..
make
./server_client_test
```

注意：测试的 CMakeLists.txt 直接编译 network 模块的源代码。

## 高层架构

### 项目结构
- `include/darwincore/` - 公共头文件（用户引用）
- `src/darwincore/` - 实现文件（.cpp）
- `src/darwincore/network/` - 内部头文件（私有实现细节）

### Network 模块架构

Network 模块实现了 **Acceptor-Reactors-Workers** 架构，用于高性能 TCP 网络通信：

#### 核心组件

**Server/Client（公共 API）**
- `darwincore::network::Server` - 面向用户的服务器接口
- `darwincore::network::Client` - 面向用户的客户端接口
- 两者都使用 Pimpl 惯用法隐藏实现细节
- 基于回调的事件处理机制（OnMessage、OnConnected 等）

**Acceptor（仅 Server 使用）**
- 运行在独立线程中
- 仅负责 `listen()` 和 `accept()`
- 使用轮询策略将新连接分配给 Reactor 线程池
- 从不对已接受的连接执行 I/O 操作

**Reactor（I/O 线程池）**
- 每个 Reactor 运行在独立线程中
- 独占管理文件描述符的所有权
- 使用 kqueue（macOS/BSD）进行 I/O 多路复用
- 执行所有套接字操作：读、写、关闭
- 将 I/O 事件转换为 `NetworkEvent` 对象
- 将事件转发给 WorkerPool 进行业务逻辑处理

**WorkerPool（业务逻辑线程池）**
- 处理来自 Reactor 的 `NetworkEvent` 对象
- 在工作线程中调用用户回调
- 从不接触文件描述符（只能看到 `connection_id`）
- `connection_id` 是 Reactor 和 Worker 之间唯一的连接纽带

#### 线程模型

**Server:**
- 1 个 Acceptor 线程（接受新连接）
- N 个 Reactor 线程（默认 = CPU 核数，处理 I/O）
- M 个 Worker 线程（可配置，执行回调）

**Client:**
- 1 个 Reactor 线程（处理 I/O）
- 1 个 Worker 线程（执行回调）

#### 关键设计原则

1. **fd 所有权**：每个 fd 仅由一个 Reactor 线程拥有
2. **业务层无 fd**：Worker 线程只能看到 `connection_id`，永远看不到原始文件描述符
3. **职责分离**：
   - Acceptor：仅接受连接
   - Reactor：仅处理 I/O
   - Worker：仅处理业务逻辑
4. **线程安全的公共 API**：所有公共方法都可以从任何线程调用
5. **基于事件的通信**：`NetworkEvent` 是层与层之间唯一的通信机制

#### 事件流程

```
客户端连接 → Acceptor.accept()
           → 分配给 Reactor（轮询）
           → Reactor 管理 fd
           → 数据到达 → Reactor 读取
           → 创建 NetworkEvent
           → 转发给 WorkerPool
           → Worker 调用用户回调（OnMessage）
```

#### 公共头文件 vs 私有头文件

**公共头文件**（`include/darwincore/network/`）：
- `client.h` - 客户端 API
- `server.h` - 服务器 API
- `event.h` - 事件类型（NetworkEvent、NetworkError、ConnectionInformation）
- `configuration.h` - 套接字配置
- `export.h` - DLL 导出宏

**私有头文件**（`src/darwincore/network/`）：
- `acceptor.h` - Acceptor 实现
- `reactor.h` - Reactor 实现
- `worker_pool.h` - 工作线程池
- `io_monitor.h` - kqueue/epoll 封装
- `socket_helper.h` - 套接字工具函数
- `concurrent_queue.h` - 线程安全队列
- `reactor_connection.h` - 内部连接结构

### Connection ID 系统

- `connection_id` 是一个 uint64_t 类型的唯一标识符
- 由 Reactor 在添加连接时生成
- 格式：`connection_id = (reactor_id << 32) | local_id`
- 业务逻辑层使用 connection_id 引用连接
- 允许从任何线程调用 `SendData()`（Reactor 会路由到正确的 fd）

### Foundation 模块

目前在构建中被禁用（参见 `src/darwincore/CMakeLists.txt`），但包含以下功能：
- 算法（BloomFilter、Hash、Search、Sort）
- 命令执行（ShellExecutor）
- 通用工具
- 容器（BitSet、CircularBuffer、LRUCache、ObjectPool 等）
- 日期/时间（DateTime、Duration、Timestamp、Calendar、TimeZone）
- 文件操作（FileManager、FilePath、FileLock、FileWatcher 等）
- 日志系统（异步、多输出、轮转）
- 内存管理
- 进程管理
- SQLite3 封装
- 字符串工具
- 线程工具（AsyncQueue、Dispatch、Lock、RunLoop）
- 限流器、定时器、UUID

## 代码风格

- C++17 标准
- 4 空格缩进（不使用 Tab）
- 每行最多 120 字符
- 类名使用大驼峰（PascalCase），如 `TCPSocketServer`、`DateTime`
- 方法名使用小驼峰（camelCase），如 `startIPServer`、`format`
- 私有成员变量带下划线前缀，如 `_server_fd`、`_running`
- 常量/宏使用全大写加下划线，如 `LOG_INFO`、`MAX_SIZE`
- 头文件保护使用 `#ifndef DARWINCORE_*_H` 模式
- 命名空间：`darwincore`，每个模块有子命名空间（如 `darwincore::network`）

## Include 模式

公共引用使用统一路径：
```cpp
#include <darwincore/network/server.h>
#include <darwincore/network/client.h>
#include <darwincore/network/event.h>
#include <darwincore/foundation/logger/Logger.h>
```

内部实现引用使用相对路径：
```cpp
#include "reactor.h"
#include "acceptor.h"
#include "worker_pool.h"
```

## 开发注意事项

- Network 模块当前处于活跃开发状态（Foundation 在构建中被注释掉）
- 项目支持 macOS 双架构构建（x86_64 + arm64）
- IOMonitor 抽象了 kqueue（macOS），未来可扩展支持 epoll（Linux）
- 根据 git status 显示，许多 network 文件已被修改（处于活跃开发阶段）
