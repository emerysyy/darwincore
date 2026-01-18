//
// DarwinCore Network 模块
// 网络事件定义
//
// 功能说明：
//   定义 Reactor 和 Worker 线程之间通信的所有网络事件类型和数据结构。
//
// 设计规则：
//   - NetworkEvent 是 IO 层和业务层之间唯一的通信语言
//   - NetworkEvent 中绝不包含 fd（使用 connection_id 代替）
//   - Worker 线程只能看到 ConnectionInformation（无 fd）
//
// 作者: DarwinCore Network 团队
// 日期: 2026

#ifndef DARWINCORE_NETWORK_EVENT_H
#define DARWINCORE_NETWORK_EVENT_H

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace darwincore {
namespace network {

/**
 * @brief 网络事件类型
 *
 * 定义网络连接上可能发生的事件类型。
 * Reactor 生成这些事件并转发给 WorkerPool。
 */
enum class NetworkEventType {
  kConnected,      ///< 新连接建立
  kData,          ///< 从对端接收到数据
  kDisconnected,  ///< 连接关闭（对端关闭或由于错误）
  kError          ///< 连接发生错误
};

/**
 * @brief 网络错误类型
 *
 * 从系统 errno 值映射的语义化错误定义。
 * 这些错误用于业务逻辑决策，而不是低级系统错误处理。
 */
enum class NetworkError {
  kPeerClosed,            ///< 对端正常关闭（FIN）
  kResetByPeer,           ///< 连接被对端重置（RST）
  kTimeout,               ///< 操作或连接超时
  kConnectionRefused,     ///< 对端拒绝连接（ECONNREFUSED）
  kNetworkUnreachable,    ///< 网络 / 主机不可达
  kProtocolViolation,     ///< 协议违规（上层使用）
  kSyscallFailure         ///< 其他系统调用错误
};


/**
 * @brief 连接信息（业务层只读视图）
 *
 * 此结构提供关于连接的信息，但不暴露文件描述符（fd）。
 * fd 只有拥有该连接的 Reactor 知道。
 *
 * 设计规则：
 *   - 不包含 fd（只有 Reactor 知道 fd）
 *   - 此结构是稳定的，可以缓存
 *   - 仅用于日志记录和业务映射
 */
struct ConnectionInformation {
  uint64_t connection_id;       ///< 唯一的连接标识符
  std::string peer_address;     ///< 对端 IP 地址或 socket 路径
  uint16_t peer_port;           ///< 对端端口号（Unix Domain 时为 0）
  bool is_unix_domain;          ///< 是否为 Unix Domain Socket

  /// 默认构造函数 - 使用无效/空值初始化
  ConnectionInformation()
      : connection_id(0), peer_port(0), is_unix_domain(false) {}

  /// 带参数的构造函数
  ConnectionInformation(uint64_t id, const std::string& address,
                        uint16_t port, bool unix_domain)
      : connection_id(id), peer_address(address), peer_port(port),
        is_unix_domain(unix_domain) {}
};

/**
 * @brief 网络事件结构
 *
 * 此结构用于在 Reactor 线程和 Worker 线程之间通信网络事件。
 * 它包含事件类型、连接 ID 和可选数据（载荷、连接信息、错误）。
 *
 * 设计规则：
 *   - NetworkEvent 是唯一的跨线程通信结构
 *   - 此结构中绝不包含 fd
 *   - 在业务逻辑中使用 connection_id 引用连接
 */
struct NetworkEvent {
  NetworkEventType type;                                ///< 事件类型
  uint64_t connection_id;                               ///< 连接 ID

  // 特定事件的数据（仅对特定事件类型有效）
  std::vector<uint8_t> payload;                         ///< 数据载荷（kData 有效）
  std::optional<ConnectionInformation> connection_info; ///< 连接信息（kConnected 有效）
  std::optional<NetworkError> error;                    ///< 错误码（kError 有效）
  std::string error_message;                            ///< 错误消息（用于日志）

  /**
   * @brief 构造函数
   * @param t 事件类型
   * @param id 连接 ID
   */
  explicit NetworkEvent(NetworkEventType t, uint64_t id)
      : type(t), connection_id(id) {}
};

}  // namespace network
}  // namespace darwincore

#endif  // DARWINCORE_NETWORK_EVENT_H
