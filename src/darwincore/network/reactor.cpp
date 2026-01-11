//
// DarwinCore Network 模块
// Reactor 实现
//
// 功能说明：
//   使用 macOS 上的 kqueue 实现 Reactor 事件循环。
//   管理文件描述符，处理 I/O 事件，并将事件转发给 WorkerPool 进行业务逻辑处理。
//
// 作者: DarwinCore Network 团队
// 日期: 2026

#include <arpa/inet.h>
#include <cstring>     // For memset, strerror
#include <errno.h>     // For errno, EAGAIN, etc.
#include <unistd.h>    // For close, read, write

#include <darwincore/network/configuration.h>
#include <darwincore/network/logger.h>
#include "socket_helper.h"
#include "io_monitor.h"
#include "reactor.h"
#include "worker_pool.h"

namespace darwincore {
namespace network {

Reactor::Reactor(int id, const std::shared_ptr<WorkerPool>& worker_pool)
    : reactor_id_(id), worker_pool_(worker_pool),
      io_monitor_(nullptr), is_running_(false),
      next_connection_id_(1) {
  io_monitor_ = new IOMonitor();
  io_monitor_->Initialize();
}

Reactor::~Reactor() {
  Stop();
  if (io_monitor_) {
    delete io_monitor_;
    io_monitor_ = nullptr;
  }
}

bool Reactor::Start() {
  if (is_running_) {
    NW_LOG_WARNING("[Reactor" << reactor_id_ << "] 已经在运行中");
    return false;
  }

  if (!io_monitor_) {
    NW_LOG_ERROR("[Reactor" << reactor_id_ << "] io_monitor_ 为空");
    return false;
  }

  is_running_ = true;
  event_loop_thread_ = std::thread(&Reactor::RunEventLoop, this);

  return true;
}

void Reactor::Stop() {
  if (!is_running_) {
    return;
  }

  is_running_ = false;

  if (event_loop_thread_.joinable()) {
    event_loop_thread_.join();
  }

  // 关闭所有连接的文件描述符
  NW_LOG_DEBUG("[Reactor" << reactor_id_ << "] 关闭 " << fd_to_connection_id_.size() << " 个连接");
  for (auto& pair : fd_to_connection_id_) {
    int fd = pair.first;
    uint64_t connection_id = pair.second;
    NW_LOG_TRACE("[Reactor" << reactor_id_ << "] 关闭 fd=" << fd << ", conn_id=" << connection_id);
    close(fd);
  }

  connections_.clear();
  fd_to_connection_id_.clear();
}

uint64_t Reactor::AddConnection(int fd, const sockaddr_storage& peer) {
  NW_LOG_DEBUG("[Reactor" << reactor_id_ << "::AddConnection] fd=" << fd << ", is_running=" << is_running_);
  if (fd < 0 || !is_running_) {
    NW_LOG_ERROR("[Reactor" << reactor_id_ << "::AddConnection] 参数无效！");
    return 0;
  }

  // 生成全局唯一的 connection_id：(reactor_id << 32) | local_id
  uint64_t local_id = next_connection_id_++;
  uint64_t connection_id = (static_cast<uint64_t>(reactor_id_) << 32) | local_id;

  // 使用 IOMonitor 开始监控 fd
  if (!io_monitor_->StartReadMonitor(fd)) {
    NW_LOG_ERROR("[Reactor" << reactor_id_ << "::AddConnection] 启动监控失败: " << strerror(errno));
    close(fd);
    return 0;
  }

  ReactorConnection conn(fd, peer, connection_id);
  connections_[connection_id] = conn;
  fd_to_connection_id_[fd] = connection_id;

  NW_LOG_DEBUG("[Reactor" << reactor_id_ << "::AddConnection] conn_id=" << connection_id << ", 提交 kConnected 事件");
  NetworkEvent connected_event(NetworkEventType::kConnected, connection_id);
  uint16_t port = 0;

  std::string ip = SocketHelper::AddressToString(peer, &port);
  bool is_unix_domain = SocketHelper::GetAddressFamily(peer) == AF_UNIX;

  connected_event.connection_info = ConnectionInformation(connection_id, ip, port, is_unix_domain);

  // 优先使用 WorkerPool 处理事件（Server 模式）
  // 如果没有 WorkerPool 或 WorkerPool 没有设置回调，则使用 event_callback_（Client 模式）
  if (worker_pool_) {
    worker_pool_->SubmitEvent(connected_event);
  } else if (event_callback_) {
    event_callback_(connected_event);
  }

  return connection_id;
}

bool Reactor::RemoveConnection(uint64_t connection_id) {
  auto it = connections_.find(connection_id);
  if (it == connections_.end()) {
    NW_LOG_WARNING("[Reactor" << reactor_id_ << "::RemoveConnection] conn_id 不存在: " << connection_id);
    return false;
  }

  int fd = it->second.file_descriptor;

  // 停止监控 fd
  if (io_monitor_) {
    io_monitor_->StopMonitor(fd);
  }

  NW_LOG_TRACE("[Reactor" << reactor_id_ << "::RemoveConnection] 关闭 fd=" << fd << ", conn_id=" << connection_id);
  close(fd);

  fd_to_connection_id_.erase(fd);
  connections_.erase(it);

  return true;
}

bool Reactor::SendData(uint64_t connection_id, const uint8_t* data, size_t size) {
  NW_LOG_TRACE("[Reactor" << reactor_id_ << "::SendData] conn_id=" << connection_id << ", size=" << size);
  auto it = connections_.find(connection_id);
  if (it == connections_.end()) {
    NW_LOG_ERROR("[Reactor" << reactor_id_ << "::SendData] conn_id 不存在！");
    return false;
  }

  int fd = it->second.file_descriptor;
  size_t sent = 0;

  while (sent < size) {
    ssize_t ret = send(fd, data + sent, size - sent, 0);
    if (ret < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        continue;
      }
      NW_LOG_ERROR("[Reactor" << reactor_id_ << "::SendData] 发送失败: " << strerror(errno));
      HandleConnectionError(it->second, errno);
      return false;
    }
    sent += static_cast<size_t>(ret);
  }

  NW_LOG_TRACE("[Reactor" << reactor_id_ << "::SendData] 发送成功: " << sent << " 字节");
  return true;
}

void Reactor::SetEventCallback(EventCallback callback) {
  event_callback_ = std::move(callback);
}

void Reactor::RunEventLoop() {
  const int kEventBatchSize = SocketConfiguration::kDefaultEventBatchSize;

  NW_LOG_INFO("[Reactor" << reactor_id_ << "] 开始事件循环");
  while (is_running_) {
    if (!io_monitor_) {
      break;
    }

#if USE_KQUEUE
    struct kevent events[kEventBatchSize];
#else
    struct epoll_event events[kEventBatchSize];
#endif

    // 设置 100ms 超时，便于检查 is_running_
    int timeout_ms = 100;
    int count = io_monitor_->WaitEvents(events, kEventBatchSize, &timeout_ms);

    if (count < 0) {
      // EINTR：系统调用被信号中断，不是真正的错误，应该继续等待
      // 例如：程序收到 SIGCHLD（子进程状态改变）或其他信号时，
      //      kevent/epoll_wait 可能会被中断返回 EINTR
      // 这种情况下应该重新调用等待，而不是退出循环
      if (errno == EINTR) {
        NW_LOG_TRACE("[Reactor" << reactor_id_ << "] WaitEvents 被信号中断 (EINTR)，重新等待");
        continue;
      }
      // 其他错误：真正的错误（如 EBADF、EFAULT 等），退出循环
      NW_LOG_ERROR("[Reactor" << reactor_id_ << "] WaitEvents 出错: " << strerror(errno) << ", 退出事件循环");
      break;
    }

    if (count > 0) {
      NW_LOG_TRACE("[Reactor" << reactor_id_ << "] 收到 " << count << " 个事件");
    }

    for (int i = 0; i < count; ++i) {
#if USE_KQUEUE
      int fd = events[i].ident;
      uint16_t flags = events[i].flags;
      int16_t filter = events[i].filter;
#else
      int fd = events[i].data.fd;
      uint32_t events_flags = events[i].events;
#endif

      NW_LOG_TRACE("[Reactor" << reactor_id_ << "] 事件: fd=" << fd);

      auto it = fd_to_connection_id_.find(fd);
      if (it == fd_to_connection_id_.end()) {
        NW_LOG_WARNING("[Reactor" << reactor_id_ << "] fd 不在映射中");
        continue;
      }

      auto conn_it = connections_.find(it->second);
      if (conn_it == connections_.end()) {
        continue;
      }

      ReactorConnection& conn = conn_it->second;

#if USE_KQUEUE
      if (flags & EV_EOF) {
        HandleConnectionClose(conn);
        continue;
      }
#else
      if (events_flags & EPOLLHUP) {
        HandleConnectionClose(conn);
        continue;
      }
#endif

      HandleReadEvent(fd);
    }
  }
  NW_LOG_INFO("[Reactor" << reactor_id_ << "] 事件循环结束");
}

void Reactor::HandleReadEvent(int fd) {
  NW_LOG_TRACE("[Reactor" << reactor_id_ << "::HandleReadEvent] fd=" << fd);
  uint8_t buffer[SocketConfiguration::kDefaultReceiveBufferSize];

  auto it = fd_to_connection_id_.find(fd);
  if (it == fd_to_connection_id_.end()) {
    NW_LOG_WARNING("[Reactor" << reactor_id_ << "::HandleReadEvent] fd 不在映射中！");
    return;
  }

  uint64_t connection_id = it->second;
  auto conn_it = connections_.find(connection_id);
  if (conn_it == connections_.end()) {
    NW_LOG_WARNING("[Reactor" << reactor_id_ << "::HandleReadEvent] conn_id 不在连接中！");
    return;
  }

    while (true) {
    ssize_t ret = recv(fd, buffer, sizeof(buffer), 0);

    if (ret > 0) {
      NW_LOG_TRACE("[Reactor" << reactor_id_ << "::HandleReadEvent] 收到 " << ret << " 字节, conn_id=" << connection_id);
      NW_LOG_DEBUG("[Reactor" << reactor_id_ << "::HandleReadEvent] 收到 " << ret << " 字节");
      NetworkEvent data_event(NetworkEventType::kData, connection_id);
      data_event.payload.assign(buffer, buffer + ret);

      // 优先使用 WorkerPool 处理事件（Server 模式）
      // 如果没有 WorkerPool，则使用 event_callback_（Client 模式）
      if (worker_pool_) {
        NW_LOG_TRACE("[Reactor" << reactor_id_ << "] 提交事件到 WorkerPool");
        worker_pool_->SubmitEvent(data_event);
      } else if (event_callback_) {
        NW_LOG_TRACE("[Reactor" << reactor_id_ << "] 使用 event_callback_");
        event_callback_(data_event);
      }
    } else if (ret == 0) {
      NW_LOG_INFO("[Reactor" << reactor_id_ << "::HandleReadEvent] 连接关闭 (ret=0)");
      HandleConnectionClose(conn_it->second);
      return;
    } else {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return;
      }
      NW_LOG_ERROR("[Reactor" << reactor_id_ << "::HandleReadEvent] 错误: " << strerror(errno));
      HandleConnectionError(conn_it->second, errno);
      return;
    }
  }
}

void Reactor::HandleConnectionClose(const ReactorConnection& conn) {
  uint64_t connection_id = conn.connection_id;

  NetworkEvent disconnected_event(NetworkEventType::kDisconnected, connection_id);

  // 优先使用 WorkerPool 处理事件（Server 模式）
  // 如果没有 WorkerPool，则使用 event_callback_（Client 模式）
  if (worker_pool_) {
    worker_pool_->SubmitEvent(disconnected_event);
  } else if (event_callback_) {
    event_callback_(disconnected_event);
  }

  RemoveConnection(connection_id);
}

void Reactor::HandleConnectionError(const ReactorConnection& conn, int error_code) {
  uint64_t connection_id = conn.connection_id;

  NetworkEvent error_event(NetworkEventType::kError, connection_id);
  error_event.error = MapErrnoToNetworkError(error_code);
  error_event.error_message = strerror(error_code);

  // 优先使用 WorkerPool 处理事件（Server 模式）
  // 如果没有 WorkerPool，则使用 event_callback_（Client 模式）
  if (worker_pool_) {
    worker_pool_->SubmitEvent(error_event);
  } else if (event_callback_) {
    event_callback_(error_event);
  }

  RemoveConnection(connection_id);
}

NetworkError Reactor::MapErrnoToNetworkError(int errno_val) {
  switch (errno_val) {
    case ECONNRESET:
      return NetworkError::kResetByPeer;
    case ETIMEDOUT:
      return NetworkError::kTimeout;
    case EPIPE:
      return NetworkError::kPeerClosed;
    default:
      return NetworkError::kSyscallFailure;
  }
}

}
}
