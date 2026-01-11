//
// LogFormatter.h
// DarwinCore
//

#ifndef DARWINCORE_LOG_FORMATTER_H
#define DARWINCORE_LOG_FORMATTER_H

#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>

namespace darwincore {
namespace log {

/// 日志级别
enum class LogLevel {
  Trace = 0,
  Debug = 1,
  Info = 2,
  Warning = 3,
  Error = 4,
  Fatal = 5
};

/// 日志级别转字符串
const char *logLevelName(LogLevel level);

/// 日志条目
struct LogEntry {
  LogLevel level = LogLevel::Info;
  std::string message;
  std::string file;
  int line = 0;
  std::string function;
  std::chrono::system_clock::time_point timestamp =
      std::chrono::system_clock::now();
  std::thread::id threadId = std::this_thread::get_id();
};

/// 日志格式化器接口
class LogFormatter {
public:
  virtual ~LogFormatter() = default;

  /// 格式化日志条目
  virtual std::string format(const LogEntry &entry) = 0;
};

/// 默认格式化器
class DefaultFormatter : public LogFormatter {
public:
  explicit DefaultFormatter(const std::string &pattern = "[%t] [%l] %m")
      : pattern_(pattern) {}

  std::string format(const LogEntry &entry) override;

private:
  std::string formatTime(std::chrono::system_clock::time_point tp);

  std::string pattern_;
};

/// JSON 格式化器
class JsonFormatter : public LogFormatter {
public:
  std::string format(const LogEntry &entry) override;

private:
  std::string formatTime(std::chrono::system_clock::time_point tp);
  std::string escapeJson(const std::string &str);
};

/// 彩色终端格式化器
class ColorFormatter : public LogFormatter {
public:
  explicit ColorFormatter(const std::string &pattern = "[%t] [%l] %m")
      : defaultFormatter_(pattern) {}

  std::string format(const LogEntry &entry) override;

private:
  std::string colorize(LogLevel level, const std::string &text);

  DefaultFormatter defaultFormatter_;
};

} // namespace log
} // namespace darwincore

#endif // DARWINCORE_LOG_FORMATTER_H
