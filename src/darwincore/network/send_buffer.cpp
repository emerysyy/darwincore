//
// DarwinCore Network 模块
// SendBuffer 实现
//
// 功能说明：
//   实现线性发送缓冲区，支持自动压缩和高水位检测。
//
// 作者: DarwinCore Network 团队
// 日期: 2026

#include <algorithm>
#include <cstring>
#include <errno.h>

#include "send_buffer.h"
#include <darwincore/network/logger.h>

namespace darwincore
{
  namespace network
  {

    SendBuffer::SendBuffer() : buffer_(INITIAL_CAPACITY), read_pos_(0), write_pos_(0)
    {
      buffer_.resize(INITIAL_CAPACITY);
    }

    bool SendBuffer::Write(const uint8_t *data, size_t size)
    {
      if (!data || size == 0)
      {
        return false;
      }

      // 确保有足够的可写空间
      if (!EnsureWritableSpace(size))
      {
        NW_LOG_ERROR("[SendBuffer] 无法分配 " << size << " 字节（超过最大容量）");
        return false;
      }

      // 直接拷贝到写位置
      std::memcpy(buffer_.data() + write_pos_, data, size);
      write_pos_ += size;

      return true;
    }

    ssize_t SendBuffer::SendToSocket(int fd)
    {
      if (fd < 0)
      {
        return -1;
      }

      size_t readable = ContiguousReadableBytes();
      if (readable == 0)
      {
        return 0;
      }

      // 尝试发送所有可读数据
      ssize_t sent = send(fd, ReadPtr(), readable, MSG_DONTWAIT | MSG_NOSIGNAL);

      if (sent > 0)
      {
        // 成功发送部分数据
        read_pos_ += sent;

        // 如果读完了，重置索引
        if (read_pos_ == write_pos_)
        {
          read_pos_ = 0;
          write_pos_ = 0;
        }
        else if (read_pos_ > buffer_.size() / 2)
        {
          // 如果读指针超过容量的一半，执行压缩
          Compact();
        }

        return sent;
      }
      else if (sent < 0)
      {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
          // 对端缓冲区满，需要等待可写事件
          return 0;
        }
        else if (errno == EINTR)
        {
          // 被信号中断，稍后重试
          return 0;
        }
        else
        {
          // 其他错误
          NW_LOG_ERROR("[SendBuffer] send() 失败: " << strerror(errno));
          return -1;
        }
      }

      // sent == 0，不应该发生
      return 0;
    }

    void SendBuffer::Compact()
    {
      if (read_pos_ == 0)
      {
        return; // 已经在开头，无需压缩
      }

      size_t readable = Size();
      if (readable > 0)
      {
        // 将剩余数据移动到开头
        std::memmove(buffer_.data(), buffer_.data() + read_pos_, readable);
      }

      read_pos_ = 0;
      write_pos_ = readable;
    }

    bool SendBuffer::IsHighWaterMark() const
    {
      return Size() >= HIGH_WATER_MARK;
    }

    bool SendBuffer::IsLowWaterMark() const
    {
      return Size() < LOW_WATER_MARK;
    }

    void SendBuffer::Clear()
    {
      read_pos_ = 0;
      write_pos_ = 0;
    }

    bool SendBuffer::EnsureWritableSpace(size_t size)
    {
      size_t writable = buffer_.size() - write_pos_;

      // 如果当前空间足够，直接返回
      if (writable >= size)
      {
        return true;
      }

      // 先尝试压缩
      if (read_pos_ > 0)
      {
        Compact();
        writable = buffer_.size() - write_pos_;
        if (writable >= size)
        {
          return true;
        }
      }

      // 需要扩容
      size_t required_size = write_pos_ + size;

      // 检查是否超过最大容量
      if (required_size > MAX_CAPACITY)
      {
        // 尝试压缩后再检查
        if (read_pos_ > 0)
        {
          Compact();
          required_size = write_pos_ + size;
        }

        if (required_size > MAX_CAPACITY)
        {
          NW_LOG_ERROR("[SendBuffer] 超过最大容量 " << MAX_CAPACITY);
          return false;
        }
      }

      // 计算新的容量（2 倍增长）
      size_t new_capacity = buffer_.size();
      while (new_capacity < required_size && new_capacity < MAX_CAPACITY)
      {
        new_capacity *= 2;
      }

      if (new_capacity > MAX_CAPACITY)
      {
        new_capacity = MAX_CAPACITY;
      }

      buffer_.resize(new_capacity);
      return true;
    }

  } // namespace network
} // namespace darwincore
