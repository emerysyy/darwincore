//
// AsyncLogger.h
// DarwinCore
//

#ifndef DARWINCORE_ASYNC_LOGGER_H
#define DARWINCORE_ASYNC_LOGGER_H

#include <darwincore/foundation/logger/LogFormatter.h>
#include <darwincore/foundation/logger/LogSink.h>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

namespace darwincore {
namespace log {

/// 异步日志器
class AsyncLogger {
public:
  explicit AsyncLogger(std::shared_ptr<LogSink> sink, size_t queueSize = 10000)
      : sink_(std::move(sink)), maxQueueSize_(queueSize), running_(false) {}

  ~AsyncLogger();

  AsyncLogger(const AsyncLogger &) = delete;
  AsyncLogger &operator=(const AsyncLogger &) = delete;

  /// 启动后台写入线程
  void start();

  /// 停止并刷新所有日志
  void stop();

  /// 异步写入日志
  void log(LogEntry entry);

  /// 便捷日志方法
  void trace(const std::string &msg) { logWithLevel(LogLevel::Trace, msg); }
  void debug(const std::string &msg) { logWithLevel(LogLevel::Debug, msg); }
  void info(const std::string &msg) { logWithLevel(LogLevel::Info, msg); }
  void warning(const std::string &msg) { logWithLevel(LogLevel::Warning, msg); }
  void error(const std::string &msg) { logWithLevel(LogLevel::Error, msg); }
  void fatal(const std::string &msg) { logWithLevel(LogLevel::Fatal, msg); }

  /// 设置最低日志级别
  void setLevel(LogLevel level) { minLevel_ = level; }

  /// 设置队列满时是否阻塞
  void setBlockWhenFull(bool block) { blockWhenFull_ = block; }

  /// 立即刷新所有待处理日志
  void flush();

  /// 获取丢弃的日志数量
  size_t droppedCount() const { return m_droppedCount; }

  /// 获取队列大小
  size_t queueSize() const;

private:
  void logWithLevel(LogLevel level, const std::string &msg);

  void workerLoop();

  std::shared_ptr<LogSink> sink_;
  size_t maxQueueSize_;
  std::atomic<bool> running_;
  std::queue<LogEntry> queue_;
  mutable std::mutex mutex_;
  std::condition_variable condition_;
  std::condition_variable fullCondition_;
  std::thread workerThread_;
  LogLevel minLevel_ = LogLevel::Trace;
  bool blockWhenFull_ = false;
  std::atomic<size_t> m_droppedCount{0};
};

/// 全局日志管理器
class LogManager {
public:
  static LogManager &instance();

  /// 配置日志器
  void configure(std::shared_ptr<LogSink> sink, bool async = true);

  void setLevel(LogLevel level) { minLevel_ = level; }

  void log(LogLevel level, const std::string &msg, const char *file = "",
           int line = 0, const char *func = "");

  void flush();

private:
  LogManager() = default;

  std::shared_ptr<LogSink> sink_;
  std::unique_ptr<AsyncLogger> asyncLogger_;
  LogLevel minLevel_ = LogLevel::Trace;
};

// 便捷宏
#define LOG_TRACE(msg)                                                         \
  darwincore::log::Logger::instance().log(                                     \
      darwincore::log::LogLevel::Trace, msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_DEBUG(msg)                                                         \
  darwincore::log::Logger::instance().log(                                     \
      darwincore::log::LogLevel::Debug, msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_INFO(msg)                                                          \
  darwincore::log::Logger::instance().log(                                     \
      darwincore::log::LogLevel::Info, msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_WARN(msg)                                                          \
  darwincore::log::Logger::instance().log(darwincore::log::LogLevel::Warning,  \
                                          msg, __FILE__, __LINE__,             \
                                          __FUNCTION__)
#define LOG_ERROR(msg)                                                         \
  darwincore::log::Logger::instance().log(                                     \
      darwincore::log::LogLevel::Error, msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_FATAL(msg)                                                         \
  darwincore::log::Logger::instance().log(                                     \
      darwincore::log::LogLevel::Fatal, msg, __FILE__, __LINE__, __FUNCTION__)

} // namespace log
} // namespace darwincore

#endif // DARWINCORE_ASYNC_LOGGER_H
