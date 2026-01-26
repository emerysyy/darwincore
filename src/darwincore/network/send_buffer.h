//
// DarwinCore Network 模块
// SendBuffer - 线性发送缓冲区
//
// 功能说明：
//   高性能的线性发送缓冲区，使用读写指针实现零拷贝发送。
//   支持自动压缩、高水位检测和最大容量限制。
//
// 设计原则：
//   - 使用读写指针而非环形缓冲区（简化设计）
//   - 自动压缩以避免无限内存增长
//   - 高水位检测用于背压控制
//
// 作者: DarwinCore Network 团队
// 日期: 2026

#ifndef DARWINCORE_NETWORK_SEND_BUFFER_H
#define DARWINCORE_NETWORK_SEND_BUFFER_H

#include <cstdint>
#include <vector>

#include <sys/socket.h>

namespace darwincore
{
  namespace network
  {

    /**
     * @brief 线性发送缓冲区
     *
     * 使用线性缓冲区（而非环形）实现，通过读写指针管理数据。
     * 当读指针超过容量一半时自动压缩，将数据移动到缓冲区开头。
     *
     * 性能特性：
     *   - Write(): O(1) append，可能触发扩容
     *   - SendToSocket(): O(1) 读取，可能触发压缩
     *   - Compact(): O(n) 内存移动，但触发频率低
     *
     * 背压控制：
     *   - 高水位（4MB）：超过时触发背压
     *   - 最大容量（16MB）：防止内存耗尽
     */
    class SendBuffer
    {
    public:
      /// 默认构造函数
      SendBuffer();

      /// 析构函数
      ~SendBuffer() = default;

      // 禁止拷贝和移动
      SendBuffer(const SendBuffer &) = delete;
      SendBuffer &operator=(const SendBuffer &) = delete;

      // 启用移动构造和移动赋值
      SendBuffer(SendBuffer &&other) noexcept
          : buffer_(std::move(other.buffer_)), read_pos_(other.read_pos_),
            write_pos_(other.write_pos_)
      {
        other.read_pos_ = 0;
        other.write_pos_ = 0;
      }

      SendBuffer &operator=(SendBuffer &&other) noexcept
      {
        if (this != &other)
        {
          buffer_ = std::move(other.buffer_);
          read_pos_ = other.read_pos_;
          write_pos_ = other.write_pos_;
          other.read_pos_ = 0;
          other.write_pos_ = 0;
        }
        return *this;
      }

      /**
       * @brief 写入数据到缓冲区
       * @param data 数据指针
       * @param size 数据大小
       * @return 成功返回 true，失败返回 false（缓冲区满）
       */
      bool Write(const uint8_t *data, size_t size);

      /**
       * @brief 将缓冲区数据发送到 socket
       * @param fd 文件描述符
       * @return 发送的字节数（> 0），0 表示 EAGAIN，-1 表示错误
       *
       * 此函数会尝试发送所有缓冲数据，直到：
       *   - 发送完毕（返回发送字节数）
       *   - 对端缓冲区满（返回 0，需要等待可写事件）
       *   - 发生错误（返回 -1）
       */
      ssize_t SendToSocket(int fd);

      /**
       * @brief 压缩缓冲区（将剩余数据移动到开头）
       *
       * 通常在以下情况调用：
       *   - 读指针超过容量一半
       *   - 需要腾出连续空间写入新数据
       */
      void Compact();

      /**
       * @brief 获取当前数据大小
       * @return 可读字节数
       */
      size_t Size() const { return write_pos_ - read_pos_; }

      /**
       * @brief 检查缓冲区是否为空
       * @return 空返回 true
       */
      bool IsEmpty() const { return read_pos_ == write_pos_; }

      /**
       * @brief 检查是否超过高水位
       * @return 超过返回 true（应触发背压）
       */
      bool IsHighWaterMark() const;

      /**
       * @brief 检查是否低于低水位
       * @return 低于返回 true（可以恢复读取）
       */
      bool IsLowWaterMark() const;

      /**
       * @brief 清空缓冲区
       */
      void Clear();

      /**
       * @brief 获取可读数据的指针
       * @return 指向可读数据的指针
       */
      const uint8_t *ReadPtr() const { return buffer_.data() + read_pos_; }

      /**
       * @brief 获取连续可读字节数
       * @return 连续可读字节数（线性缓冲区 = Size()）
       */
      size_t ContiguousReadableBytes() const { return Size(); }

      /**
       * @brief 获取当前容量
       * @return 缓冲区总容量
       */
      size_t Capacity() const { return buffer_.size(); }

    private:
      /**
       * @brief 确保有足够的可写空间
       * @param size 需要的空间大小
       * @return 成功返回 true，失败返回 false（超过最大容量）
       */
      bool EnsureWritableSpace(size_t size);

      std::vector<uint8_t> buffer_; ///< 底层存储
      size_t read_pos_{0};          ///< 读位置
      size_t write_pos_{0};         ///< 写位置

      // 常量配置
      static constexpr size_t INITIAL_CAPACITY = 4096;            // 4KB 初始容量
      static constexpr size_t HIGH_WATER_MARK = 8 * 1024 * 1024;  // 8MB 高水位（提高背压阈值）
      static constexpr size_t LOW_WATER_MARK = 4 * 1024 * 1024;   // 4MB 低水位
      static constexpr size_t MAX_CAPACITY = 32 * 1024 * 1024;    // 32MB 最大容量
    };

  } // namespace network
} // namespace darwincore

#endif // DARWINCORE_NETWORK_SEND_BUFFER_H
