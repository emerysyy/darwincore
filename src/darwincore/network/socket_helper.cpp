//
// DarwinCore Network 模块
// Socket 辅助函数实现
//
// 功能说明：
//   实现 Socket 创建、配置、地址解析等辅助功能。
//
// 作者: DarwinCore Network 团队
// 日期: 2026

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include <darwincore/network/configuration.h>
#include <darwincore/network/logger.h>
#include "socket_helper.h"

namespace darwincore
{
  namespace network
  {

    int SocketHelper::CreateSocket(SocketProtocol protocol)
    {
      int domain = AF_INET;
      const char *protocol_str = "IPv4";

      switch (protocol)
      {
      case SocketProtocol::kIPv4:
        domain = AF_INET;
        protocol_str = "IPv4";
        break;
      case SocketProtocol::kIPv6:
        domain = AF_INET6;
        protocol_str = "IPv6";
        break;
      case SocketProtocol::kUniversalIP:
        domain = AF_INET6;
        protocol_str = "UniversalIP";
        break;
      case SocketProtocol::kUnixDomain:
        domain = AF_UNIX;
        protocol_str = "UnixDomain";
        break;
      }

      NW_LOG_DEBUG("[SocketHelper::CreateSocket] 创建 socket，协议=" << protocol_str);
      int fd = socket(domain, SOCK_STREAM, 0);

      if (fd < 0)
      {
        NW_LOG_ERROR("[SocketHelper::CreateSocket] 创建 socket 失败: " << strerror(errno)
                                                                       << ", 协议=" << protocol_str);
      }
      else
      {
        NW_LOG_TRACE("[SocketHelper::CreateSocket] socket 创建成功，fd=" << fd << ", 协议=" << protocol_str);
      }

      return fd;
    }

    bool SocketHelper::SetNonBlocking(int fd)
    {
      NW_LOG_TRACE("[SocketHelper::SetNonBlocking] 设置 fd=" << fd << " 为非阻塞模式");

      int flags = fcntl(fd, F_GETFL, 0);
      if (flags < 0)
      {
        NW_LOG_ERROR("[SocketHelper::SetNonBlocking] F_GETFL 失败: " << strerror(errno)
                                                                     << ", fd=" << fd);
        return false;
      }

      int ret = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
      if (ret < 0)
      {
        NW_LOG_ERROR("[SocketHelper::SetNonBlocking] F_SETFL 失败: " << strerror(errno)
                                                                     << ", fd=" << fd);
        return false;
      }

      NW_LOG_TRACE("[SocketHelper::SetNonBlocking] fd=" << fd << " 非阻塞设置成功");
      return true;
    }

    bool SocketHelper::SetSocketOption(int fd, int level, int optname,
                                       const void *optval, socklen_t optlen)
    {
      int ret = setsockopt(fd, level, optname, optval, optlen);

      if (ret < 0)
      {
        NW_LOG_WARNING("[SocketHelper::SetSocketOption] setsockopt 失败: " << strerror(errno)
                                                                           << ", fd=" << fd << ", level=" << level << ", optname=" << optname);
      }

      return ret >= 0;
    }

    bool SocketHelper::ResolveAddress(const std::string &host,
                                      uint16_t port,
                                      SocketProtocol protocol,
                                      sockaddr_storage *addr)
    {
      NW_LOG_DEBUG("[SocketHelper::ResolveAddress] 解析地址: " << host << ":" << port
                                                               << ", 协议=" << static_cast<int>(protocol));

      if (addr == nullptr)
      {
        NW_LOG_ERROR("[SocketHelper::ResolveAddress] addr 参数为空");
        return false;
      }

      memset(addr, 0, sizeof(sockaddr_storage));

      if (protocol == SocketProtocol::kUnixDomain)
      {
        NW_LOG_TRACE("[SocketHelper::ResolveAddress] Unix Domain Socket: " << host);
        // Unix Domain Socket
        auto *unix_addr = reinterpret_cast<sockaddr_un *>(addr);
        unix_addr->sun_family = AF_UNIX;
        strncpy(unix_addr->sun_path, host.c_str(), sizeof(unix_addr->sun_path) - 1);
        NW_LOG_DEBUG("[SocketHelper::ResolveAddress] Unix Domain Socket 解析成功: " << host);
        return true;
      }

      if (protocol == SocketProtocol::kIPv4)
      {
        NW_LOG_TRACE("[SocketHelper::ResolveAddress] 解析 IPv4 地址: " << host);
        // IPv4
        auto *addr4 = reinterpret_cast<sockaddr_in *>(addr);
        addr4->sin_family = AF_INET;
        addr4->sin_port = htons(port);
        if (inet_pton(AF_INET, host.c_str(), &addr4->sin_addr) != 1)
        {
          NW_LOG_ERROR("[SocketHelper::ResolveAddress] IPv4 地址解析失败: " << host);
          return false;
        }
        NW_LOG_DEBUG("[SocketHelper::ResolveAddress] IPv4 地址解析成功: " << host);
        return true;
      }
      else if (protocol == SocketProtocol::kIPv6)
      {
        NW_LOG_TRACE("[SocketHelper::ResolveAddress] 解析 IPv6 地址: " << host);
        // IPv6
        auto *addr6 = reinterpret_cast<sockaddr_in6 *>(addr);
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = htons(port);
        if (inet_pton(AF_INET6, host.c_str(), &addr6->sin6_addr) != 1)
        {
          NW_LOG_ERROR("[SocketHelper::ResolveAddress] IPv6 地址解析失败: " << host);
          return false;
        }
        NW_LOG_DEBUG("[SocketHelper::ResolveAddress] IPv6 地址解析成功: " << host);
        return true;
      }
      else
      {
        NW_LOG_ERROR("[SocketHelper::ResolveAddress] UniversalIP 协议不支持直接解析");
        // UniversalIP - 不支持直接解析，需要分别创建 IPv4 和 IPv6 socket
        return false;
      }
    }

    std::string SocketHelper::AddressToString(const sockaddr_storage &addr,
                                              uint16_t *port)
    {
      char buf[INET6_ADDRSTRLEN] = {};

      if (addr.ss_family == AF_INET)
      {
        const auto *addr4 = reinterpret_cast<const sockaddr_in *>(&addr);
        inet_ntop(AF_INET, &addr4->sin_addr, buf, sizeof(buf));
        if (port != nullptr)
        {
          *port = ntohs(addr4->sin_port);
        }
        NW_LOG_TRACE("[SocketHelper::AddressToString] IPv4: " << buf << ":" << ntohs(addr4->sin_port));
      }
      else if (addr.ss_family == AF_INET6)
      {
        const auto *addr6 = reinterpret_cast<const sockaddr_in6 *>(&addr);
        inet_ntop(AF_INET6, &addr6->sin6_addr, buf, sizeof(buf));
        if (port != nullptr)
        {
          *port = ntohs(addr6->sin6_port);
        }
        NW_LOG_TRACE("[SocketHelper::AddressToString] IPv6: " << buf << ":" << ntohs(addr6->sin6_port));
      }
      else if (addr.ss_family == AF_UNIX)
      {
        const auto *unix_addr = reinterpret_cast<const sockaddr_un *>(&addr);
        NW_LOG_TRACE("[SocketHelper::AddressToString] Unix Domain: " << unix_addr->sun_path);
        return std::string(unix_addr->sun_path);
      }
      else
      {
        NW_LOG_WARNING("[SocketHelper::AddressToString] 未知地址族: " << addr.ss_family);
        return "unknown";
      }

      return std::string(buf);
    }

    int SocketHelper::GetAddressFamily(const sockaddr_storage &addr)
    {
      return addr.ss_family;
    }

    bool SocketHelper::IsIPv4MappedIPv6(const sockaddr_storage &addr)
    {
      if (addr.ss_family != AF_INET6)
      {
        return false;
      }

      const auto *addr6 = reinterpret_cast<const sockaddr_in6 *>(&addr);
      const uint8_t *bytes = addr6->sin6_addr.s6_addr;

      // IPv4-mapped IPv6: ::ffff:x.x.x.x
      bool is_mapped = (bytes[0] == 0 && bytes[1] == 0 && bytes[2] == 0 && bytes[3] == 0 &&
                        bytes[4] == 0 && bytes[5] == 0 && bytes[6] == 0 && bytes[7] == 0 &&
                        bytes[8] == 0 && bytes[9] == 0 && bytes[10] == 0xff && bytes[11] == 0xff);

      if (is_mapped)
      {
        NW_LOG_TRACE("[SocketHelper::IsIPv4MappedIPv6] 检测到 IPv4 映射的 IPv6 地址");
      }

      return is_mapped;
    }

    bool SocketHelper::SetUnixDomainAddress(const std::string &path,
                                            sockaddr_un *addr)
    {
      NW_LOG_TRACE("[SocketHelper::SetUnixDomainAddress] 设置 Unix Domain 地址: " << path);

      if (addr == nullptr)
      {
        NW_LOG_ERROR("[SocketHelper::SetUnixDomainAddress] addr 参数为空");
        return false;
      }

      memset(addr, 0, sizeof(sockaddr_un));
      addr->sun_family = AF_UNIX;

      size_t path_len = path.length();
      if (path_len >= sizeof(addr->sun_path))
      {
        NW_LOG_ERROR("[SocketHelper::SetUnixDomainAddress] 路径过长: " << path
                                                                       << ", 长度=" << path_len << ", 最大=" << sizeof(addr->sun_path) - 1);
        return false;
      }

      strncpy(addr->sun_path, path.c_str(), sizeof(addr->sun_path) - 1);
      NW_LOG_TRACE("[SocketHelper::SetUnixDomainAddress] Unix Domain 地址设置成功");
      return true;
    }

    void SocketHelper::UnlinkUnixDomainSocket(const std::string &path)
    {
      NW_LOG_DEBUG("[SocketHelper::UnlinkUnixDomainSocket] 删除 socket 文件: " << path);

      struct stat st;
      if (lstat(path.c_str(), &st) == 0)
      {
        if (S_ISSOCK(st.st_mode))
        {
          if (unlink(path.c_str()) == 0)
          {
            NW_LOG_INFO("[SocketHelper::UnlinkUnixDomainSocket] Socket 文件已删除: " << path);
          }
          else
          {
            NW_LOG_WARNING("[SocketHelper::UnlinkUnixDomainSocket] 删除失败: " << path
                                                                               << ": " << strerror(errno));
          }
        }
        else
        {
          NW_LOG_WARNING("[SocketHelper::UnlinkUnixDomainSocket] 文件存在但不是 socket: " << path);
        }
      }
      else
      {
        NW_LOG_TRACE("[SocketHelper::UnlinkUnixDomainSocket] 文件不存在: " << path);
      }
    }

    std::string SocketHelper::GetErrorMessage(int err)
    {
      return strerror(err);
    }

    bool SocketHelper::IsRetryError(int err)
    {
      return (err == EAGAIN || err == EWOULDBLOCK);
    }

  } // namespace network
} // namespace darwincore
