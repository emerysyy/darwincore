//
// CircularBuffer.h
// DarwinCore
//
// Created by DarwinCore on 2026/1/7.
// Copyright © 2026 DarwinCore. All rights reserved.
//

#ifndef DARWINCORE_CIRCULAR_BUFFER_H
#define DARWINCORE_CIRCULAR_BUFFER_H

#include <algorithm>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <vector>

namespace darwincore {
namespace container {

/**
 * @brief 环形缓冲区（Circular Buffer / Ring Buffer）
 *
 * 实用开发建议与使用场景：
 * 1.
 * 实时流处理：非常适合音频或视频原始数据的平滑处理。当处理速度波动时，它可以作为缓冲层，防止播放卡顿。
 * 2.
 * 低波动日志采集：在生产环境中，可以使用它暂存高频日志，然后由后台线程批量写入磁盘（减少系统调用开销）。
 * 3. 传感器采样：在一个固定的内存池里保存最近的 N
 * 个采样点（如温度、心率），用于滑动窗口平均值计算。
 * 4. 驱动程序通信：在底层驱动接收数据时，作为 DMA
 * 或中断请求的直接缓冲区，减少复制。
 *
 * 注意：
 * -
 * 本实现默认覆盖旧数据，这在追求时效性的场景（如最新路况、最新报价）中非常有用。
 * - 该类非线程安全，若需在并发环境使用，请配合互斥锁或使用无锁队列替代方案。
 *
 * @tparam T 存储元素的类型
 */
template <typename T> class CircularBuffer {
public:
  /**
   * @brief 构造函数
   * @param capacity 缓冲区容量（必须大于0）
   * @throws std::invalid_argument 如果容量为0
   */
  explicit CircularBuffer(size_t capacity)
      : buffer_(capacity), capacity_(capacity), head_(0), tail_(0), size_(0) {
    if (capacity == 0) {
      throw std::invalid_argument(
          "CircularBuffer capacity must be greater than 0");
    }
  }

  /**
   * @brief 默认析构函数
   */
  ~CircularBuffer() = default;

  // 禁用拷贝构造和拷贝赋值（可以根据需要启用）
  CircularBuffer(const CircularBuffer &) = delete;
  CircularBuffer &operator=(const CircularBuffer &) = delete;

  // 支持移动语义
  CircularBuffer(CircularBuffer &&other) noexcept
      : buffer_(std::move(other.buffer_)), capacity_(other.capacity_),
        head_(other.head_), tail_(other.tail_), size_(other.size_) {
    other.capacity_ = 0;
    other.head_ = 0;
    other.tail_ = 0;
    other.size_ = 0;
  }

  CircularBuffer &operator=(CircularBuffer &&other) noexcept {
    if (this != &other) {
      buffer_ = std::move(other.buffer_);
      capacity_ = other.capacity_;
      head_ = other.head_;
      tail_ = other.tail_;
      size_ = other.size_;
      other.capacity_ = 0;
      other.head_ = 0;
      other.tail_ = 0;
      other.size_ = 0;
    }
    return *this;
  }

  /**
   * @brief 向缓冲区添加元素
   *
   * 如果缓冲区已满，最旧的元素会被覆盖。
   *
   * @param value 要添加的元素
   * @return true 表示成功添加，false 表示覆盖了旧元素
   */
  bool push(const T &value) {
    bool overwritten = false;

    if (full()) {
      // 缓冲区满，移动 head 指针（覆盖最旧元素）
      head_ = (head_ + 1) % capacity_;
      overwritten = true;
    } else {
      ++size_;
    }

    buffer_[tail_] = value;
    tail_ = (tail_ + 1) % capacity_;

    return !overwritten;
  }

  /**
   * @brief 向缓冲区添加元素（移动语义）
   */
  bool push(T &&value) {
    bool overwritten = false;

    if (full()) {
      head_ = (head_ + 1) % capacity_;
      overwritten = true;
    } else {
      ++size_;
    }

    buffer_[tail_] = std::move(value);
    tail_ = (tail_ + 1) % capacity_;

    return !overwritten;
  }

  /**
   * @brief 从缓冲区取出最旧的元素
   * @return 包含元素的 optional，如果缓冲区为空则返回 std::nullopt
   */
  std::optional<T> pop() {
    if (empty()) {
      return std::nullopt;
    }

    T value = std::move(buffer_[head_]);
    head_ = (head_ + 1) % capacity_;
    --size_;

    return value;
  }

  /**
   * @brief 查看最旧的元素（不移除）
   * @return 指向元素的指针，如果缓冲区为空则返回 nullptr
   */
  const T *front() const {
    if (empty()) {
      return nullptr;
    }
    return &buffer_[head_];
  }

  /**
   * @brief 查看最新的元素（不移除）
   * @return 指向元素的指针，如果缓冲区为空则返回 nullptr
   */
  const T *back() const {
    if (empty()) {
      return nullptr;
    }
    size_t index = (tail_ + capacity_ - 1) % capacity_;
    return &buffer_[index];
  }

  /**
   * @brief 通过索引访问元素
   * @param index 从head开始的偏移量（0表示最旧元素）
   * @return 元素引用
   * @throws std::out_of_range 如果索引超出范围
   */
  const T &at(size_t index) const {
    if (index >= size_) {
      throw std::out_of_range("CircularBuffer index out of range");
    }
    return buffer_[(head_ + index) % capacity_];
  }

  T &at(size_t index) {
    if (index >= size_) {
      throw std::out_of_range("CircularBuffer index out of range");
    }
    return buffer_[(head_ + index) % capacity_];
  }

  /**
   * @brief 下标运算符（不进行边界检查）
   */
  const T &operator[](size_t index) const {
    return buffer_[(head_ + index) % capacity_];
  }

  T &operator[](size_t index) { return buffer_[(head_ + index) % capacity_]; }

  /**
   * @brief 清空缓冲区
   */
  void clear() {
    head_ = 0;
    tail_ = 0;
    size_ = 0;
  }

  /**
   * @brief 检查缓冲区是否为空
   */
  [[nodiscard]] bool empty() const noexcept { return size_ == 0; }

  /**
   * @brief 检查缓冲区是否已满
   */
  [[nodiscard]] bool full() const noexcept { return size_ == capacity_; }

  /**
   * @brief 获取当前元素数量
   */
  [[nodiscard]] size_t size() const noexcept { return size_; }

  /**
   * @brief 获取缓冲区容量
   */
  [[nodiscard]] size_t capacity() const noexcept { return capacity_; }

  /**
   * @brief 获取剩余可用空间
   */
  [[nodiscard]] size_t available() const noexcept { return capacity_ - size_; }

  // ========================================
  // 迭代器支持
  // ========================================

  class iterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T *;
    using reference = T &;

    iterator(CircularBuffer *buffer, size_t pos, size_t count)
        : buffer_(buffer), pos_(pos), count_(count) {}

    reference operator*() { return buffer_->buffer_[pos_]; }
    pointer operator->() { return &buffer_->buffer_[pos_]; }

    iterator &operator++() {
      pos_ = (pos_ + 1) % buffer_->capacity_;
      ++count_;
      return *this;
    }

    iterator operator++(int) {
      iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    bool operator==(const iterator &other) const {
      return count_ == other.count_;
    }

    bool operator!=(const iterator &other) const { return !(*this == other); }

  private:
    CircularBuffer *buffer_;
    size_t pos_;
    size_t count_;
  };

  class const_iterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = const T *;
    using reference = const T &;

    const_iterator(const CircularBuffer *buffer, size_t pos, size_t count)
        : buffer_(buffer), pos_(pos), count_(count) {}

    reference operator*() const { return buffer_->buffer_[pos_]; }
    pointer operator->() const { return &buffer_->buffer_[pos_]; }

    const_iterator &operator++() {
      pos_ = (pos_ + 1) % buffer_->capacity_;
      ++count_;
      return *this;
    }

    const_iterator operator++(int) {
      const_iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    bool operator==(const const_iterator &other) const {
      return count_ == other.count_;
    }

    bool operator!=(const const_iterator &other) const {
      return !(*this == other);
    }

  private:
    const CircularBuffer *buffer_;
    size_t pos_;
    size_t count_;
  };

  iterator begin() { return iterator(this, head_, 0); }
  iterator end() { return iterator(this, tail_, size_); }
  const_iterator begin() const { return const_iterator(this, head_, 0); }
  const_iterator end() const { return const_iterator(this, tail_, size_); }
  const_iterator cbegin() const { return const_iterator(this, head_, 0); }
  const_iterator cend() const { return const_iterator(this, tail_, size_); }

private:
  std::vector<T> buffer_; // 底层存储
  size_t capacity_;       // 缓冲区容量
  size_t head_;           // 读取位置（最旧元素）
  size_t tail_;           // 写入位置（下一个写入位置）
  size_t size_;           // 当前元素数量
};

} // namespace container
} // namespace darwincore

#endif // DARWINCORE_CIRCULAR_BUFFER_H
