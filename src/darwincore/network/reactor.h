//
// DarwinCore Network 模块
// Reactor - IO 事件循环
//
// 功能说明：
//   Reactor 负责管理文件描述符和 I/O 事件。
//   在自己的线程中运行，使用 kqueue (macOS/FreeBSD) 或 epoll (Linux)。
//
// 设计规则：
//   - 每个 fd 仅属于一个 Reactor 线程
//   - Reactor 执行所有 socket 操作（读、写、关闭）
//   - 将 I/O 结果转换为 NetworkEvents 并转发给 WorkerPool
//   - 绝不与 Worker 线程或业务逻辑共享 fd
//
// 作者: DarwinCore Network 团队
// 日期: 2026

#ifndef DARWINCORE_NETWORK_REACTOR_H
#define DARWINCORE_NETWORK_REACTOR_H

#include <cstdint>
#include <functional>
#include <memory>
#include <thread>
#include <unordered_map>

#include <darwincore/network/event.h>  // 对外暴露的头文件

namespace darwincore {
namespace network {

// 前向声明
class WorkerPool;
class IOMonitor;

/**
 * @brief Reactor - IO 事件循环
 *
 * Reactor 在专用线程中管理文件描述符和 I/O 事件。
 * 职责包括：
 *   - 添加/移除连接
 *   - 从 sockets 读/写数据
 *   - 将 I/O 事件转换为 NetworkEvents
 *   - 将事件转发给 WorkerPool 进行业务逻辑处理
 *
 * 线程安全：
 *   - Reactor 在自己的线程中运行
 *   - 公共方法是线程安全的（可以从任何线程调用）
 *   - 私有方法仅从 Reactor 线程调用
 */
class Reactor {
public:
  /// 事件回调类型别名
  using EventCallback = std::function<void(const NetworkEvent&)>;

  /**
   * @brief 构造新的 Reactor 对象
   * @param id Reactor ID（用于日志/调试）
   * @param worker_pool WorkerPool 的共享指针，用于事件转发
   */
  Reactor(int id, const std::shared_ptr<WorkerPool>& worker_pool);

  /// 析构函数 - 停止事件循环
  ~Reactor();

  // 不可拷贝和不可移动
  Reactor(const Reactor&) = delete;
  Reactor& operator=(const Reactor&) = delete;

  /**
   * @brief 启动 reactor 事件循环
   * @return 启动成功返回 true，否则返回 false
   */
  bool Start();

  /**
   * @brief 停止 reactor 事件循环
   */
  void Stop();

  /**
   * @brief 向 reactor 添加新连接
   * @param fd 连接的文件描述符
   * @param peer 对端地址信息
   * @return connection_id（如果添加成功），0（如果失败）
   *
   * 生成的 connection_id 格式：(reactor_id << 32) | local_id
   */
  uint64_t AddConnection(int fd, const sockaddr_storage& peer);

  /**
   * @brief 从 reactor 中移除连接
   * @param connection_id 要移除的连接 ID
   * @return 移除成功返回 true，否则返回 false
   */
  bool RemoveConnection(uint64_t connection_id);

  /**
   * @brief 向连接发送数据
   * @param connection_id 发送数据的连接 ID
   * @param data 数据缓冲区指针
   * @param size 要发送的数据大小
   * @return 发送成功返回 true，否则返回 false
   */
  bool SendData(uint64_t connection_id, const uint8_t* data, size_t size);

  /**
   * @brief 设置接收网络事件的事件回调
   * @param callback 当网络事件发生时调用的函数
   */
  void SetEventCallback(EventCallback callback);

  /**
   * @brief 获取 reactor ID
   * @return Reactor ID
   */
  int GetReactorId() const { return reactor_id_; }

private:
  // 内部连接结构（在 reactor.cpp 中定义）
  struct ReactorConnection {
    int file_descriptor;
    sockaddr_storage peer;
    uint64_t connection_id;

    ReactorConnection() : file_descriptor(-1), connection_id(0) {
      memset(&peer, 0, sizeof(peer));
    }

    ReactorConnection(int fd, const sockaddr_storage& p, uint64_t id)
        : file_descriptor(fd), peer(p), connection_id(id) {}
  };

  /**
   * @brief 在专用线程中运行的主事件循环
   */
  void RunEventLoop();

  /**
   * @brief 处理来自连接的读事件
   * @param file_descriptor 有数据要读的文件描述符
   */
  void HandleReadEvent(int file_descriptor);

  /**
   * @brief 处理连接关闭事件
   * @param connection 连接信息
   */
  void HandleConnectionClose(const ReactorConnection& connection);

  /**
   * @brief 处理连接错误事件
   * @param connection 连接信息
   * @param error_code 系统错误代码（errno）
   */
  void HandleConnectionError(const ReactorConnection& connection, int error_code);

  /**
   * @brief 将系统 errno 映射到 NetworkError 枚举
   * @param errno_val 系统 errno 值
   * @return 对应的 NetworkError 值
   */
  NetworkError MapErrnoToNetworkError(int errno_val);

  // 成员变量
  int reactor_id_;                                    ///< Reactor ID 用于日志
  std::shared_ptr<WorkerPool> worker_pool_;          ///< WorkerPool 用于事件转发
  EventCallback event_callback_;                     ///< 事件回调函数

  IOMonitor* io_monitor_;                             ///< IO 监控器（内部管理）
  std::thread event_loop_thread_;                    ///< 事件循环线程
  bool is_running_;                                  ///< Reactor 运行状态

  uint64_t next_connection_id_;                      ///< 下一个要分配的连接 ID
  std::unordered_map<uint64_t, ReactorConnection> connections_;        ///< connection_id -> connection
  std::unordered_map<int, uint64_t> fd_to_connection_id_;             ///< fd -> connection_id 映射
};

}  // namespace network
}  // namespace darwincore

#endif  // DARWINCORE_NETWORK_REACTOR_H
