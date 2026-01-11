//
// RingQueue.h
// DarwinCore
//

#ifndef DARWINCORE_RING_QUEUE_H
#define DARWINCORE_RING_QUEUE_H

#include <atomic>
#include <cstddef>
#include <optional>
#include <vector>

namespace darwincore {
namespace container {

  /**
   * @brief Single-Producer Single-Consumer lock-free ring queue.
   *
   * 线程模型（强约束）：
   * - Exactly ONE producer thread
   * - Exactly ONE consumer thread
   * - 任意多于 1 个 producer 或 consumer → **未定义行为**
   *
   * 设计目标：
   * - 极低延迟（无锁、无 CAS）
   * - 高吞吐（仅原子 load/store）
   * - 避免 false sharing（cache line 对齐）
   *
   * 行为语义：
   * - push / pop 永不阻塞
   * - 队列满时 push 返回 false
   * - 队列空时 pop 返回 std::nullopt
   * - 不提供等待 / 超时 / 唤醒机制
   *
   * 容量说明：
   * - 构造参数 capacity 表示“期望容量”
   * - 实际内部容量会被向上对齐到 2 的幂
   * - 可存放的最大元素数量为 (capacity_ - 1)
   *
   * 适用场景（推荐）：
   * - macOS EndpointSecurity 回调线程 → 策略处理线程
   * - Network Extension / DPI 抓包线程 → 协议解析线程
   * - 实时日志、事件、消息流水线
   *
   * 不适用场景（禁止）：
   * - 多生产者或多消费者
   * - 需要阻塞等待或条件变量
   * - 数据不可丢失的业务逻辑
   *
   * @tparam T Element type. Should be movable and cheap to destruct.
   */
template <typename T> class SPSCRingQueue {
public:
  explicit SPSCRingQueue(size_t capacity)
      : capacity_(nextPowerOfTwo(capacity)), mask_(capacity_ - 1),
        buffer_(new T[capacity_]), head_(0), tail_(0) {}

  ~SPSCRingQueue() { delete[] buffer_; }

  SPSCRingQueue(const SPSCRingQueue &) = delete;
  SPSCRingQueue &operator=(const SPSCRingQueue &) = delete;

  bool push(const T &value) {
    const size_t currentTail = tail_.load(std::memory_order_relaxed);
    const size_t nextTail = (currentTail + 1) & mask_;
    if (nextTail == head_.load(std::memory_order_acquire))
      return false;
    buffer_[currentTail] = value;
    tail_.store(nextTail, std::memory_order_release);
    return true;
  }

  bool push(T &&value) {
    const size_t currentTail = tail_.load(std::memory_order_relaxed);
    const size_t nextTail = (currentTail + 1) & mask_;
    if (nextTail == head_.load(std::memory_order_acquire))
      return false;
    buffer_[currentTail] = std::move(value);
    tail_.store(nextTail, std::memory_order_release);
    return true;
  }

  std::optional<T> pop() {
    const size_t currentHead = head_.load(std::memory_order_relaxed);
    if (currentHead == tail_.load(std::memory_order_acquire))
      return std::nullopt;
    T value = std::move(buffer_[currentHead]);
    head_.store((currentHead + 1) & mask_, std::memory_order_release);
    return value;
  }

  [[nodiscard]] bool empty() const {
    return head_.load(std::memory_order_acquire) ==
           tail_.load(std::memory_order_acquire);
  }

  [[nodiscard]] size_t capacity() const noexcept { return capacity_ - 1; }

private:
  static size_t nextPowerOfTwo(size_t n) {
    if (n == 0)
      return 1;
    --n;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    return n + 1;
  }

  const size_t capacity_;
  const size_t mask_;
  T *buffer_;
  alignas(64) std::atomic<size_t> head_;
  alignas(64) std::atomic<size_t> tail_;
};

  /**
   * @brief Multi-Producer Multi-Consumer bounded lock-free ring queue.
   *
   * 算法来源：
   * - Dmitry Vyukov's bounded MPMC queue
   *
   * 线程模型：
   * - Multiple producers
   * - Multiple consumers
   *
   * 行为语义：
   * - push / pop 永不阻塞
   * - 队列满时 push 返回 false
   * - 队列空时 pop 返回 std::nullopt
   *
   * 设计特点：
   * - 使用 per-slot sequence number 解决 ABA 问题
   * - 支持多线程高并发访问
   * - 性能显著优于 mutex + queue
   *
   * 注意事项（非常重要）：
   * - capacity 必须是 2 的幂（内部自动对齐）
   * - T 应避免复杂析构或超大对象
   * - 不保证公平性（可能短暂饥饿）
   *
   * 适用场景：
   * - 线程池任务分发
   * - 安全审计任务队列
   * - 多源事件 → 多处理线程
   */
template <typename T> class MPMCRingQueue {
public:
  explicit MPMCRingQueue(size_t capacity)
      : capacity_(nextPowerOfTwo(capacity)), mask_(capacity_ - 1),
        buffer_(new Cell[capacity_]), head_(0), tail_(0) {
    for (size_t i = 0; i < capacity_; ++i)
      buffer_[i].sequence.store(i, std::memory_order_relaxed);
  }

  ~MPMCRingQueue() { delete[] buffer_; }

  bool push(const T &value) {
    Cell *cell;
    size_t pos = tail_.load(std::memory_order_relaxed);
    for (;;) {
      cell = &buffer_[pos & mask_];
      size_t seq = cell->sequence.load(std::memory_order_acquire);
      auto diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);
      if (diff == 0) {
        if (tail_.compare_exchange_weak(pos, pos + 1,
                                        std::memory_order_relaxed))
          break;
      } else if (diff < 0)
        return false;
      else
        pos = tail_.load(std::memory_order_relaxed);
    }
    cell->data = value;
    cell->sequence.store(pos + 1, std::memory_order_release);
    return true;
  }

  std::optional<T> pop() {
    Cell *cell;
    size_t pos = head_.load(std::memory_order_relaxed);
    for (;;) {
      cell = &buffer_[pos & mask_];
      size_t seq = cell->sequence.load(std::memory_order_acquire);
      auto diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos + 1);
      if (diff == 0) {
        if (head_.compare_exchange_weak(pos, pos + 1,
                                        std::memory_order_relaxed))
          break;
      } else if (diff < 0)
        return std::nullopt;
      else
        pos = head_.load(std::memory_order_relaxed);
    }
    T data = std::move(cell->data);
    cell->sequence.store(pos + mask_ + 1, std::memory_order_release);
    return data;
  }

private:
  struct Cell {
    std::atomic<size_t> sequence;
    T data;
  };
  static size_t nextPowerOfTwo(size_t n) {
    if (n == 0)
      return 1;
    --n;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    return n + 1;
  }
  const size_t capacity_, mask_;
  Cell *buffer_;
  alignas(64) std::atomic<size_t> head_;
  alignas(64) std::atomic<size_t> tail_;
};

} // namespace container
} // namespace darwincore

#endif // DARWINCORE_RING_QUEUE_H
