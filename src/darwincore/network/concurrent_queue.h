//
// DarwinCore Network Module
// Thread-Safe Queue for Events（带容量限制）
//
// Description:
//   A thread-safe queue implementation using mutex and condition variable.
//   Used by WorkerPool to receive NetworkEvents from Reactors.
//   Supports capacity limits for backpressure control.
//
// Thread Safety:
//   - All operations are thread-safe
//   - Multiple threads can enqueue and dequeue concurrently
//
// Author: DarwinCore Network Team
// Date: 2026

#ifndef DARWINCORE_NETWORK_CONCURRENT_QUEUE_H
#define DARWINCORE_NETWORK_CONCURRENT_QUEUE_H

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>

namespace darwincore {
namespace network {

/**
 * @brief Thread-safe queue for inter-thread communication with capacity limits
 *
 * This template class provides a thread-safe queue that can be used
 * to pass data between threads. It uses a mutex to protect access
 * and a condition variable to notify waiting threads.
 *
 * New features:
 *   - Optional capacity limit for backpressure control
 *   - TryEnqueue() for non-blocking enqueue
 *   - Size() and IsFull() for monitoring
 *
 * Usage Example:
 *   @code
 *   ConcurrentQueue<int> queue(1000);  // Max 1000 items
 *   queue.Enqueue(42);                 // Blocking enqueue
 *
 *   if (queue.TryEnqueue(43)) {        // Non-blocking enqueue
 *     // Successfully enqueued
 *   }
 *
 *   int value;
 *   if (queue.TryDequeue(value)) {
 *     // Successfully dequeued
 *   }
 *   @endcode
 *
 * @tparam T Type of elements stored in the queue
 */
template <typename T> class ConcurrentQueue {
public:
  /// Default constructor (unlimited capacity)
  ConcurrentQueue() : max_size_(0), is_stopped_(false) {}

  /// Constructor with capacity limit
  /// @param max_size Maximum queue size (0 = unlimited)
  explicit ConcurrentQueue(size_t max_size) : max_size_(max_size), is_stopped_(false) {}

  /// Destructor
  ~ConcurrentQueue() = default;

  // Non-copyable and non-movable
  ConcurrentQueue(const ConcurrentQueue &) = delete;
  ConcurrentQueue &operator=(const ConcurrentQueue &) = delete;

  /**
   * @brief Enqueue an element to the queue (blocking)
   * @param value Value to enqueue (thread-safe)
   *
   * If queue is full, blocks until space is available.
   */
bool Enqueue(const T& value) {
  {
    std::unique_lock<std::mutex> lock(mutex_);

    if (max_size_ > 0) {
      not_full_.wait(lock, [this] {
        return queue_.size() < max_size_ || is_stopped_;
      });
    }

    if (is_stopped_) {
      return false;
    }

    queue_.push(value);
  }

  not_empty_.notify_one();
  return true;
}


  /**
   * @brief Try to enqueue an element to the queue (non-blocking)
   * @param value Value to enqueue
   * @return true if enqueued successfully, false if queue is full
   *
   * This is the key method for backpressure control.
   * Returns false immediately if queue is full.
   */
  bool TryEnqueue(const T &value) {
    {
      std::lock_guard<std::mutex> lock(mutex_);

      // Check if queue is full
      if (max_size_ > 0 && queue_.size() >= max_size_) {
        return false; // Queue is full
      }

      if (is_stopped_) {
        return false; // Don't enqueue if stopped
      }

      queue_.push(value);
    }
    not_empty_.notify_one();
    return true;
  }

  /**
   * @brief Try to dequeue an element from the queue (non-blocking)
   * @param result Reference to store the dequeued value
   * @return true if an element was dequeued, false if queue is empty
   */
  bool TryDequeue(T &result) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) {
      return false;
    }
    result = std::move(queue_.front());
    queue_.pop();

    // Notify one waiting enqueuer that space is available
    if (max_size_ > 0) {
      not_full_.notify_one();
    }

    return true;
  }

  /**
   * @brief Wait and dequeue an element with timeout (blocking)
   * @param result Reference to store the dequeued value
   * @param timeout Maximum time to wait
   * @return true if an element was dequeued, false if timeout or stopped
   *
   * This method blocks until:
   *   - An element is available in the queue
   *   - The timeout expires
   *   - NotifyStop() is called
   */
  bool WaitDequeue(T &result, std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(mutex_);

    bool success = not_empty_.wait_for(lock, timeout, [this] {
      return !queue_.empty() || is_stopped_;
    });

    if (!success || is_stopped_ || queue_.empty()) {
      return false;
    }

    result = std::move(queue_.front());
    queue_.pop();

    // Notify one waiting enqueuer that space is available
    if (max_size_ > 0) {
      not_full_.notify_one();
    }

    return true;
  }

  /**
   * @brief Notify all waiting threads to stop
   *
   * Call this method to wake up all threads waiting in WaitDequeue.
   * After calling this, WaitDequeue will return false immediately.
   */
  void NotifyStop() {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      is_stopped_ = true;
    }
    not_empty_.notify_all();
    not_full_.notify_all();
  }

  /**
   * @brief Reset the stopped state
   *
   * Call this to allow WaitDequeue to work again after NotifyStop.
   */
  void Reset() {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      is_stopped_ = false;
    }
    // Note: We don't notify here as the queue is being reset
  }

  /**
   * @brief Check if the queue is empty
   * @return true if queue is empty, false otherwise
   */
  bool IsEmpty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
  }

  /**
   * @brief Get the current queue size
   * @return Number of elements in the queue
   */
  size_t Size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
  }

  /**
   * @brief Check if the queue is full
   * @return true if queue is full (and has a limit), false otherwise
   */
  bool IsFull() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return max_size_ > 0 && queue_.size() >= max_size_;
  }

  /**
   * @brief Check if the queue is stopped
   * @return true if stopped, false otherwise
   */
  bool IsStopped() const { return is_stopped_; }

  /**
   * @brief Get the maximum queue size
   * @return Maximum queue size (0 = unlimited)
   */
  size_t GetMaxSize() const { return max_size_; }

private:
  mutable std::mutex mutex_;        ///< Mutex for thread synchronization
  std::queue<T> queue_;             ///< Underlying queue
  std::condition_variable not_empty_;   ///< Condition variable for dequeue
  std::condition_variable not_full_;    ///< Condition variable for enqueue (when limited)
  std::atomic<bool> is_stopped_;    ///< Flag to indicate stop notification
  size_t max_size_;                 ///< Maximum queue size (0 = unlimited)
};

} // namespace network
} // namespace darwincore

#endif // DARWINCORE_NETWORK_CONCURRENT_QUEUE_H
