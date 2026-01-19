//
// DarwinCore Network 模块
// Socket 辅助函数
//
// 功能说明：
//   提供 Socket 创建、配置、地址解析等辅助功能。
//   包括设置非阻塞、地址转换、错误处理等。
//
// 作者: DarwinCore Network 团队
// 日期: 2026

#ifndef DARWINCORE_NETWORK_SOCKET_HELPER_H
#define DARWINCORE_NETWORK_SOCKET_HELPER_H

#include <cstdint>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>

#include <darwincore/network/configuration.h>

namespace darwincore
{
  namespace network
  {

    /**
     * @brief Socket 辅助函数类
     *
     * 提供静态方法用于 Socket 操作的辅助功能。
     * 所有方法都是静态的，不需要实例化对象。
     */
    class SocketHelper
    {
    public:
      // ==================== Socket 创建和配置 ====================

      /**
       * @brief 创建 Socket
       * @param protocol Socket 协议类型
       * @return Socket 文件描述符，失败返回 -1
       *
       * 根据 SocketProtocol 创建对应类型的 Socket：
       * - IPv4: socket(AF_INET, SOCK_STREAM, 0)
       * - IPv6: socket(AF_INET6, SOCK_STREAM, 0)
       * - UnixDomain: socket(AF_UNIX, SOCK_STREAM, 0)
       */
      static int CreateSocket(SocketProtocol protocol);

      /**
       * @brief 设置 Socket 为非阻塞模式
       * @param fd Socket 文件描述符
       * @return 成功返回 true，失败返回 false
       *
       * 使用 fcntl 设置 O_NONBLOCK 标志。
       */
      static bool SetNonBlocking(int fd);

      /**
       * @brief 设置 Socket 选项
       * @param fd Socket 文件描述符
       * @param level 协议级别（如 SOL_SOCKET）
       * @param optname 选项名称（如 SO_REUSEADDR）
       * @param optval 选项值指针
       * @param optlen 选项值长度
       * @return 成功返回 true，失败返回 false
       */
      static bool SetSocketOption(int fd, int level, int optname,
                                  const void *optval, socklen_t optlen);

      // ==================== 地址解析和转换 ====================

      /**
       * @brief 将主机名/IP 地址解析为 sockaddr_storage
       * @param host 主机名或 IP 地址
       * @param port 端口号
       * @param protocol Socket 协议类型
       * @param addr 输出参数，存储解析后的地址
       * @return 成功返回 true，失败返回 false
       *
       * 支持的格式：
       * - IPv4: "192.168.1.1", "0.0.0.0"
       * - IPv6: "::1", "[::]", "fe80::1"
       * - 域名: "example.com"（会进行 DNS 解析）
       */
      static bool ResolveAddress(const std::string &host,
                                 uint16_t port,
                                 SocketProtocol protocol,
                                 sockaddr_storage *addr);

      /**
       * @brief 将 sockaddr_storage 转换为字符串地址
       * @param addr Socket 地址
       * @param port 输出参数，存储端口号
       * @return 字符串形式的 IP 地址
       *
       * 对于 IPv4 返回 "192.168.1.1" 格式
       * 对于 IPv6 返回 "::1" 格式
       * 对于 Unix Domain 返回 socket 路径
       */
      static std::string AddressToString(const sockaddr_storage &addr,
                                         uint16_t *port = nullptr);

      /**
       * @brief 获取地址族（IPv4/IPv6/Unix）
       * @param addr Socket 地址
       * @return 地址族（AF_INET, AF_INET6, 或 AF_UNIX）
       */
      static int GetAddressFamily(const sockaddr_storage &addr);

      /**
       * @brief 检查是否为 IPv4 映射的 IPv6 地址
       * @param addr Socket 地址
       * @return 如果是 IPv4-mapped IPv6 地址返回 true
       *
       * IPv4-mapped IPv6 地址格式为 ::ffff:x.x.x.x
       */
      static bool IsIPv4MappedIPv6(const sockaddr_storage &addr);

      // ==================== Unix Domain Socket 特殊处理 ====================

      /**
       * @brief 设置 Unix Domain Socket 地址
       * @param path Socket 路径
       * @param addr 输出参数，存储 Unix 地址结构
       * @return 成功返回 true，失败返回 false
       *
       * 如果路径长度超过 sun_path 上限（104 bytes），
       * 会尝试使用相对路径配合 chdir 策略。
       */
      static bool SetUnixDomainAddress(const std::string &path,
                                       sockaddr_un *addr);

      /**
       * @brief 删除 Unix Domain Socket 文件
       * @param path Socket 文件路径
       *
       * 在绑定新的 Unix Domain Socket 之前，
       * 应该先删除旧的 socket 文件（如果存在）。
       */
      static void UnlinkUnixDomainSocket(const std::string &path);

      // ==================== 错误处理 ====================

      /**
       * @brief 获取最后一次 Socket 错误的描述
       * @param err 错误码（errno）
       * @return 错误描述字符串
       */
      static std::string GetErrorMessage(int err);

      /**
       * @brief 判断是否为"重试"错误（非阻塞模式下的 EAGAIN/EWOULDBLOCK）
       * @param err 错误码
       * @return 如果是可重试错误返回 true
       */
      static bool IsRetryError(int err);
    };

  } // namespace network
} // namespace darwincore

#endif // DARWINCORE_NETWORK_SOCKET_HELPER_H
