//
//  Created by Darwin Core on 2023/1/18.
//

#ifndef OperationQueue_hpp
#define OperationQueue_hpp

#include <condition_variable>
#include <functional>
#include <future> // std::packaged_task
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace darwincore {
namespace thread {
// ========================================
// 异步队列类（线程池）
// ========================================
// 功能：
// 1. 提供异步任务执行能力（类似线程池）
// 2. 支持任务队列的并发处理
// 3. 使用单例模式，全局共享
// ========================================
class AsyncQueue {

private:
  bool isRunning;                        // 运行标志
  int mQueueSize;                        // 线程池大小
  std::mutex mQueueMutex;                // 任务队列互斥锁
  std::condition_variable mConditionVar; // 条件变量，用于线程同步
  std::queue<std::function<void(void)>> mTaskQueue; // 任务队列
  std::vector<std::thread> mThreadArray;            // 线程数组

public:
  // 获取默认队列大小
  static int defaultQueueSize();

  // 获取单例实例
  static AsyncQueue &queue();

public:
  // 构造函数
  explicit AsyncQueue(int queueSize);
  ~AsyncQueue();

  // 提交异步任务
  // F: 函数类型
  // Args: 参数类型
  template <typename F, typename... Args> void submit(F &&f, Args &&...args);

private:
  AsyncQueue() = default;

  // 初始化线程池
  void initial();
};
}; // namespace thread
}; // namespace darwincore

// 模板函数实现
template <typename F, typename... Args>
void darwincore::thread::AsyncQueue::submit(F &&f, Args &&...args) {

  // 获取函数返回值类型
  // c++ 11 新特性：使用 std::result_of 获取返回值类型
  using fReturnType = typename std::result_of<F(Args...)>::type;
  // 创建 packaged_task，支持异步获取返回值
  auto task = std::make_shared<std::packaged_task<fReturnType(void)>>(
      std::bind(std::forward<F>(f), std::forward<Args>(args)...));

  {
    // 加锁，将任务加入队列
    std::unique_lock<std::mutex> lock(this->mQueueMutex);
    if (!this->isRunning) {
      return;
    }
    this->mTaskQueue.emplace([task]() { (*task)(); });
  }
  // 通知一个工作线程有新任务
  this->mConditionVar.notify_one();
}

#endif /* OperationQueue_hpp */
