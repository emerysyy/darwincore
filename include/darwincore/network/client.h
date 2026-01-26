//
// DarwinCore Network 模块
// 网络客户端 - 统一客户端接口
//
// 功能说明：
//   提供统一的网络客户端接口，支持 IPv4、IPv6 和 Unix Domain Socket 连接。
//   内部使用 Reactor-Worker 架构，自动管理连接和事件分发。
//
// 设计规则：
//   - Client 是用户唯一需要关心的接口
//   - 内部实现（Reactor、Worker）对用户透明
//   - 使用回调函数处理网络事件
//
// 使用示例：
//   @code
//   darwincore::network::Client client;
//
//   // 设置事件回调
//   client.SetOnConnected([](const auto& info) {
//     std::cout << "已连接到服务器" << std::endl;
//   });
//
//   client.SetOnMessage([](const auto& data) {
//     // 处理接收到的数据
//   });
//
//   // 连接到服务器
//   client.ConnectIPv4("127.0.0.1", 8080);
//
//   // 发送数据
//   const uint8_t data[] = {1, 2, 3, 4};
//   client.SendData(data, sizeof(data));
//
//   // ... 使用连接 ...
//
//   client.Disconnect();
//   @endcode
//
// 作者: DarwinCore Network 团队
// 日期: 2026

#ifndef DARWINCORE_NETWORK_CLIENT_H
#define DARWINCORE_NETWORK_CLIENT_H

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include <darwincore/network/event.h>

namespace darwincore
{
  namespace network
  {

    /**
     * @brief 网络客户端类
     *
     * 提供统一的客户端接口，支持多种 Socket 类型。
     * 内部自动管理 Reactor 线程（I/O）和 Worker 线程（业务）。
     *
     * 支持的连接方式：
     * - ConnectIPv4: 连接到 IPv4 服务器
     * - ConnectIPv6: 连接到 IPv6 服务器
     * - ConnectUnixDomain: 连接到 Unix Domain Socket 服务器
     *
     * 线程模型：
     * - 1 个 Reactor 线程：负责 I/O 操作
     * - 1 个 Worker 线程：负责业务逻辑和回调处理
     *
     * 注意事项：
     * - Client 一次只能保持一个连接
     * - 在连接建立之前调用 SendData 会失败
     * - 所有回调都在 Worker 线程中调用
     */
    class Client
    {
    public:
      /// 连接成功回调函数类型
      using OnConnectedCallback = std::function<void(const ConnectionInformation &)>;

      /// 消息接收回调函数类型
      /// @param data 接收到的数据
      using OnMessageCallback = std::function<void(const std::vector<uint8_t> &data)>;

      /// 断开连接回调函数类型
      using OnDisconnectedCallback = std::function<void()>;

      /// 连接错误回调函数类型
      /// @param error 错误类型
      /// @param message 错误消息（仅用于日志）
      using OnErrorCallback = std::function<void(NetworkError error,
                                                 const std::string &message)>;

      /**
       * @brief 构造 Client 对象
       *
       * 默认创建 1 个 Reactor 线程和 1 个 Worker 线程。
       */
      Client();

      /**
       * @brief 析构函数
       *
       * 自动断开连接并清理资源。
       */
      ~Client();

      // 禁止拷贝和移动
      Client(const Client &) = delete;
      Client &operator=(const Client &) = delete;

      // ==================== 连接管理 ====================

      /**
       * @brief 连接到 IPv4 服务器
       * @param host 服务器地址（IP 或域名）
       * @param port 服务器端口
       * @return 连接成功返回 true，失败返回 false
       *
       * 使用 IPv4 socket 连接到指定地址和端口。
       * 如果已经连接，此方法会先断开旧连接。
       */
      bool ConnectIPv4(const std::string &host, uint16_t port);

      /**
       * @brief 连接到 IPv6 服务器
       * @param host 服务器地址（IPv6 地址或域名）
       * @param port 服务器端口
       * @return 连接成功返回 true，失败返回 false
       *
       * 使用 IPv6 socket 连接到指定地址和端口。
       */
      bool ConnectIPv6(const std::string &host, uint16_t port);

      /**
       * @brief 连接到 Unix Domain Socket 服务器
       * @param path Socket 文件路径（绝对路径推荐）
       * @return 连接成功返回 true，失败返回 false
       *
       * 使用 Unix Domain Socket 连接到指定路径。
       */
      bool ConnectUnixDomain(const std::string &path);

      /**
       * @brief 优雅关闭连接（等待发送缓冲区清空）
       * @param timeout_ms 超时时间（毫秒），0表示无限等待
       * @return 成功返回 true，超时返回 false
       *
       * 此方法会：
       * 1. 停止接受新的SendData调用
       * 2. 等待SendBuffer中的数据发送完成
       * 3. 关闭连接
       *
       * 正确的使用模式：
       * @code
       *   // 1. 用户代码先停止发送源（如定时器、发送线程）
       *   stop_sending.store(true);
       *   sender_thread.join();
       *
       *   // 2. 调用GracefulShutdown等待SendBuffer清空
       *   client.GracefulShutdown(5000);  // 等待最多5秒
       *
       *   // 3. Client内部会自动清理Reactor和WorkerPool
       * @endcode
       */
      bool GracefulShutdown(int timeout_ms = 5000);

      /**
       * @brief 立即断开连接
       *
       * 关闭当前连接，清理资源。
       * 如果没有连接，此方法不执行任何操作。
       *
       * 注意：此方法会立即停止Reactor和WorkerPool，
       *       调用前请确保已经停止所有发送操作。
       */
      void Disconnect();

      // ==================== 发送数据 ====================

      /**
       * @brief 同步发送数据到服务器（默认）
       * @param data 数据指针
       * @param size 数据大小
       * @param timeout_ms 超时时间（毫秒），0表示无限等待
       * @return 发送成功返回 true，失败/超时返回 false
       *
       * 此方法会等待数据实际发送到对端后返回。
       * ClientReactor 会确保数据写入 socket 缓冲区后才返回。
       *
       * 适用场景：
       *   - 需要确保数据已发送
       *   - 流控场景
       *   - 关键数据传输
       *
       * 注意：此方法可能阻塞调用线程，建议在专用线程中使用。
       */
      bool SendData(const uint8_t *data, size_t size, int timeout_ms = 0);

      /**
       * @brief 异步发送数据到服务器（带回调）
       * @param data 数据指针
       * @param size 数据大小
       * @param callback 发送完成回调函数
       * @return 发送成功返回 true，失败返回 false
       *
       * 此方法将数据放入发送缓冲区后立即返回，不等待实际发送完成。
       * 发送完成后会在Reactor线程中调用回调函数。
       *
       * 回调函数签名：
       *   void callback(bool success, size_t bytes_sent)
       *   - success: 是否发送成功
       *   - bytes_sent: 实际发送的字节数
       *
       * 适用场景：
       *   - 高吞吐量场景
       *   - 不需要立即确认的发送
       *   - 批量发送
       *
       * 注意：返回true只表示数据已放入缓冲区，不表示已发送到对端。
       */
      using SendAsyncCallback = std::function<void(bool success, size_t bytes_sent)>;
      bool SendAsync(const uint8_t *data, size_t size, SendAsyncCallback callback);

      /**
       * @brief 获取发送缓冲区大小
       * @return 发送缓冲区中的字节数
       *
       * 可用于实现应用层流控。
       */
      size_t GetSendBufferSize() const;

      // ==================== 状态查询 ====================

      /**
       * @brief 检查是否已连接
       * @return 已连接返回 true，否则返回 false
       *
       * 此方法可以在任何线程中调用。
       */
      bool IsConnected() const;

      // ==================== 设置回调 ====================

      /**
       * @brief 设置连接成功回调
       * @param callback 连接建立成功时调用的函数
       *
       * 当连接建立成功后，此回调会在 Worker 线程中被调用。
       * 回调参数包含连接信息（不含 fd）。
       */
      void SetOnConnected(OnConnectedCallback callback);

      /**
       * @brief 设置消息接收回调
       * @param callback 接收到数据时调用的函数
       *
       * 当从服务器接收到数据后，此回调会在 Worker 线程中被调用。
       * 注意：此回调应该快速处理，避免阻塞 Worker 线程。
       */
      void SetOnMessage(OnMessageCallback callback);

      /**
       * @brief 设置断开连接回调
       * @param callback 连接断开时调用的函数
       *
       * 当连接断开后，此回调会在 Worker 线程中被调用。
       * 包括正常断开和异常断开。
       */
      void SetOnDisconnected(OnDisconnectedCallback callback);

      /**
       * @brief 设置连接错误回调
       * @param callback 连接发生错误时调用的函数
       *
       * 当连接发生错误后，此回调会在 Worker 线程中被调用。
       * 错误消息仅用于日志记录，业务决策应使用 NetworkError 枚举。
       */
      void SetOnError(OnErrorCallback callback);

    private:
      /// Pimpl 实现（隐藏内部细节）
      class Impl;
      std::unique_ptr<Impl> impl_;
    };

  } // namespace network
} // namespace darwincore

#endif // DARWINCORE_NETWORK_CLIENT_H
