//
// LogSink.h
// DarwinCore
//

#ifndef DARWINCORE_LOG_SINK_H
#define DARWINCORE_LOG_SINK_H

#include <darwincore/foundation/logger/LogFormatter.h>
#include <darwincore/foundation/logger/LogRotate.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#ifdef __APPLE__
#include <os/log.h>
#endif

namespace darwincore {
namespace log {

/// 日志输出目标接口
class LogSink {
public:
  virtual ~LogSink() = default;

  /// 写入日志
  virtual void write(const LogEntry &entry) = 0;

  /// 刷新缓冲
  virtual void flush() = 0;

  /// 设置格式化器
  void setFormatter(std::shared_ptr<LogFormatter> formatter) {
    formatter_ = std::move(formatter);
  }

  /// 设置最低日志级别
  void setLevel(LogLevel level) { minLevel_ = level; }

  /// 检查是否应该记录
  bool shouldLog(LogLevel level) const {
    return static_cast<int>(level) >= static_cast<int>(minLevel_);
  }

protected:
  std::shared_ptr<LogFormatter> formatter_ =
      std::make_shared<DefaultFormatter>();
  LogLevel minLevel_ = LogLevel::Trace;
};

/// 控制台输出
class ConsoleSink : public LogSink {
public:
  explicit ConsoleSink(bool useColors = true);

  void write(const LogEntry &entry) override;
  void flush() override;

private:
  std::mutex mutex_;
};

/// 文件输出
class FileSink : public LogSink {
public:
  FileSink(const std::string &path, bool append = true);
  FileSink(const std::string &path, RotateConfig rotateConfig);
  ~FileSink() override;

  void write(const LogEntry &entry) override;
  void flush() override;

private:
  std::string path_;
  std::ofstream file_;
  std::unique_ptr<LogRotate> rotator_;
  std::mutex mutex_;
};

#ifdef __APPLE__
/// macOS 系统日志输出
class OSLogSink : public LogSink {
public:
  explicit OSLogSink(const std::string &subsystem = "",
                     const std::string &category = "default");

  void write(const LogEntry &entry) override;
  void flush() override;

private:
  os_log_t osLog_;
};
#endif

/// 多目标组合输出
class MultiSink : public LogSink {
public:
  void addSink(std::shared_ptr<LogSink> sink) {
    sinks_.push_back(std::move(sink));
  }

  void write(const LogEntry &entry) override;
  void flush() override;

private:
  std::vector<std::shared_ptr<LogSink>> sinks_;
};

/// 回调输出
class CallbackSink : public LogSink {
public:
  using Callback = std::function<void(const LogEntry &)>;

  explicit CallbackSink(Callback callback) : callback_(std::move(callback)) {}

  void write(const LogEntry &entry) override;
  void flush() override {}

private:
  Callback callback_;
};

} // namespace log
} // namespace darwincore

#endif // DARWINCORE_LOG_SINK_H
