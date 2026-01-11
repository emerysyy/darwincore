//
// DarwinCore Network 模块
// Socket 配置定义
//
// 功能说明：
//   定义建立 Socket 连接所需的所有配置参数。
//   支持 IPv4、IPv6、双栈监听和 Unix Domain Socket。
//
// 作者: DarwinCore Network 团队
// 日期: 2026

#ifndef DARWINCORE_NETWORK_CONFIGURATION_H
#define DARWINCORE_NETWORK_CONFIGURATION_H

#include <cstdint>
#include <string>

namespace darwincore {
namespace network {

/**
 * @brief Socket 协议类型
 *
 * 定义支持的 Socket 协议族。
 * - IPv4: 仅支持 IPv4 连接
 * - IPv6: 仅支持 IPv6 连接（不映射 IPv4）
 * - UniversalIP: 同时监听 IPv4 和 IPv6（两个独立 socket）
 * - UnixDomain: Unix Domain Socket（本地 IPC）
 */
enum class SocketProtocol {
  kIPv4,          ///< IPv4 协议
  kIPv6,          ///< IPv6 协议（不映射 IPv4）
  kUniversalIP,   ///< 双栈监听（IPv4 + IPv6，独立 socket）
  kUnixDomain     ///< Unix Domain Socket
};

/**
 * @brief Socket 连接配置
 *
 * 包含建立 Socket 连接所需的所有配置参数。
 *
 * 使用示例：
 *   @code
 *   SocketConfiguration config;
 *   config.protocol = SocketProtocol::kIPv4;
 *   config.host = "0.0.0.0";
 *   config.port = 8080;
 *   config.backlog = 128;
 *   @endcode
 */
struct SocketConfiguration {
  /// Socket 协议类型
  SocketProtocol protocol;

  /// 监听地址（IPv4/IPv6）或 Socket 路径（Unix Domain）
  std::string host;

  /// 监听端口号（TCP/UDP，Unix Domain 时为 0）
  uint16_t port;

  /// listen() 的 backlog 参数（队列中最大挂起连接数）
  int backlog;

  /// 默认 Worker 线程数（用于 Server/Client 初始化）
  static constexpr size_t kDefaultWorkerCount = 4;

  /// 默认事件批量大小（Reactor 一次处理的事件数量）
  static constexpr int kDefaultEventBatchSize = 64;

  /// 默认接收缓冲区大小
  static constexpr size_t kDefaultReceiveBufferSize = 8192;

  /**
   * @brief 默认构造函数
   *
   * 默认配置为 IPv4 监听所有接口，端口 0，backlog 128。
   */
  SocketConfiguration()
      : protocol(SocketProtocol::kIPv4),
        host("0.0.0.0"),
        port(0),
        backlog(128) {}

  /**
   * @brief 便捷构造函数 - IPv4
   * @param host IPv4 地址
   * @param port 端口号
   * @param backlog 监听队列长度
   */
  static SocketConfiguration IPv4(const std::string& host,
                                     uint16_t port,
                                     int backlog = 128) {
    SocketConfiguration config;
    config.protocol = SocketProtocol::kIPv4;
    config.host = host;
    config.port = port;
    config.backlog = backlog;
    return config;
  }

  /**
   * @brief 便捷构造函数 - IPv6
   * @param host IPv6 地址
   * @param port 端口号
   * @param backlog 监听队列长度
   */
  static SocketConfiguration IPv6(const std::string& host,
                                     uint16_t port,
                                     int backlog = 128) {
    SocketConfiguration config;
    config.protocol = SocketProtocol::kIPv6;
    config.host = host;
    config.port = port;
    config.backlog = backlog;
    return config;
  }

  /**
   * @brief 便捷构造函数 - 双栈监听
   * @param host IP 地址（会同时创建 IPv4 和 IPv6 socket）
   * @param port 端口号
   * @param backlog 监听队列长度
   */
  static SocketConfiguration UniversalIP(const std::string& host,
                                          uint16_t port,
                                          int backlog = 128) {
    SocketConfiguration config;
    config.protocol = SocketProtocol::kUniversalIP;
    config.host = host;
    config.port = port;
    config.backlog = backlog;
    return config;
  }

  /**
   * @brief 便捷构造函数 - Unix Domain Socket
   * @param path Socket 文件路径（绝对路径）
   * @param backlog 监听队列长度
   */
  static SocketConfiguration UnixDomain(const std::string& path,
                                          int backlog = 128) {
    SocketConfiguration config;
    config.protocol = SocketProtocol::kUnixDomain;
    config.host = path;
    config.port = 0;
    config.backlog = backlog;
    return config;
  }
};

}  // namespace network
}  // namespace darwincore

#endif  // DARWINCORE_NETWORK_CONFIGURATION_H
