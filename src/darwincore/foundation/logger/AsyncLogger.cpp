//
// AsyncLogger.cpp
// DarwinCore
//

#include <darwincore/foundation/logger/AsyncLogger.h>
#include <vector>

namespace darwincore {
namespace log {

AsyncLogger::~AsyncLogger() {
  stop();
}

void AsyncLogger::start() {
  if (running_)
    return;
  running_ = true;
  workerThread_ = std::thread([this] { workerLoop(); });
}

void AsyncLogger::stop() {
  if (!running_)
    return;
  running_ = false;
  condition_.notify_one();
  if (workerThread_.joinable()) {
    workerThread_.join();
  }
  flush();
}

void AsyncLogger::log(LogEntry entry) {
  if (!running_) {
    // 未启动时同步写入
    sink_->write(entry);
    return;
  }

  std::unique_lock<std::mutex> lock(mutex_);

  // 如果队列满了，等待或丢弃
  if (queue_.size() >= maxQueueSize_) {
    if (blockWhenFull_) {
      fullCondition_.wait(lock,
                          [this] { return queue_.size() < maxQueueSize_; });
    } else {
      ++m_droppedCount;
      return;
    }
  }

  queue_.push(std::move(entry));
  lock.unlock();
  condition_.notify_one();
}

void AsyncLogger::flush() {
  std::unique_lock<std::mutex> lock(mutex_);
  while (!queue_.empty()) {
    LogEntry entry = std::move(queue_.front());
    queue_.pop();
    lock.unlock();
    sink_->write(entry);
    lock.lock();
  }
  sink_->flush();
}

void AsyncLogger::workerLoop() {
  while (running_ || !queue_.empty()) {
    std::unique_lock<std::mutex> lock(mutex_);

    condition_.wait(lock, [this] { return !queue_.empty() || !running_; });

    // 批量处理
    std::vector<LogEntry> batch;
    while (!queue_.empty() && batch.size() < 100) {
      batch.push_back(std::move(queue_.front()));
      queue_.pop();
    }

    bool wasFull = queue_.size() >= maxQueueSize_ - batch.size();
    lock.unlock();

    if (wasFull) {
      fullCondition_.notify_all();
    }

    // 写入日志
    for (const auto &entry : batch) {
      sink_->write(entry);
    }
  }
}

// ========================================
// LogManager Singleton Implementation
// ========================================

void LogManager::configure(std::shared_ptr<LogSink> sink, bool async) {
  sink_ = std::move(sink);
  if (async) {
    asyncLogger_ = std::unique_ptr<AsyncLogger>(new AsyncLogger(sink_));
    asyncLogger_->start();
  }
}

void LogManager::log(LogLevel level, const std::string &msg, const char *file,
                     int line, const char *func) {
  if (static_cast<int>(level) < static_cast<int>(minLevel_))
    return;

  LogEntry entry;
  entry.level = level;
  entry.message = msg;
  entry.file = file;
  entry.line = line;
  entry.function = func;

  if (asyncLogger_) {
    asyncLogger_->log(std::move(entry));
  } else if (sink_) {
    sink_->write(entry);
  }
}

void LogManager::flush() {
  if (asyncLogger_)
    asyncLogger_->flush();
  else if (sink_)
    sink_->flush();
}

} // namespace log
} // namespace darwincore
