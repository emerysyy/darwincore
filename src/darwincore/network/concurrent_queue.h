//
// DarwinCore Network Module
// Thread-Safe Queue for Events
//
// Description:
//   A thread-safe queue implementation using mutex and condition variable.
//   Used by WorkerPool to receive NetworkEvents from Reactors.
//
// Thread Safety:
//   - All operations are thread-safe
//   - Multiple threads can enqueue and dequeue concurrently
//
// Author: DarwinCore Network Team
// Date: 2026

#ifndef DARWINCORE_NETWORK_CONCURRENT_QUEUE_H
#define DARWINCORE_NETWORK_CONCURRENT_QUEUE_H

#include <condition_variable>
#include <mutex>
#include <queue>

namespace darwincore {
namespace network {

/**
 * @brief Thread-safe queue for inter-thread communication
 *
 * This template class provides a thread-safe queue that can be used
 * to pass data between threads. It uses a mutex to protect access
 * and a condition variable to notify waiting threads.
 *
 * Usage Example:
 *   @code
 *   ConcurrentQueue<int> queue;
 *   queue.Enqueue(42);  // Thread-safe enqueue
 *
 *   int value;
 *   if (queue.TryDequeue(value)) {
 *     // Successfully dequeued
 *   }
 *   @endcode
 *
 * @tparam T Type of elements stored in the queue
 */
template <typename T>
class ConcurrentQueue {
public:
  /// Default constructor
  ConcurrentQueue() = default;

  /// Destructor
  ~ConcurrentQueue() = default;

  // Non-copyable and non-movable
  ConcurrentQueue(const ConcurrentQueue&) = delete;
  ConcurrentQueue& operator=(const ConcurrentQueue&) = delete;

  /**
   * @brief Enqueue an element to the queue
   * @param value Value to enqueue (thread-safe)
   */
  void Enqueue(const T& value) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      queue_.push(value);
    }
    condition_variable_.notify_one();
  }

  /**
   * @brief Try to dequeue an element from the queue
   * @param result Reference to store the dequeued value
   * @return true if an element was dequeued, false if queue is empty
   */
  bool TryDequeue(T& result) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) {
      return false;
    }
    result = queue_.front();
    queue_.pop();
    return true;
  }

  /**
   * @brief Check if the queue is empty
   * @return true if queue is empty, false otherwise
   */
  bool IsEmpty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
  }

private:
  mutable std::mutex mutex_;                ///< Mutex for thread synchronization
  std::queue<T> queue_;                     ///< Underlying queue
  std::condition_variable condition_variable_;  ///< Condition variable for notifications
};

}  // namespace network
}  // namespace darwincore

#endif  // DARWINCORE_NETWORK_CONCURRENT_QUEUE_H
