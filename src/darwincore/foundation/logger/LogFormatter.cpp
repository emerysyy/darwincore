//
// LogFormatter.cpp
// DarwinCore
//

#include <darwincore/foundation/logger/LogFormatter.h>
#include <ctime>

namespace darwincore {
namespace log {

const char *logLevelName(LogLevel level) {
  switch (level) {
  case LogLevel::Trace:
    return "TRACE";
  case LogLevel::Debug:
    return "DEBUG";
  case LogLevel::Info:
    return "INFO";
  case LogLevel::Warning:
    return "WARN";
  case LogLevel::Error:
    return "ERROR";
  case LogLevel::Fatal:
    return "FATAL";
  }
  return "UNKNOWN";
}

std::string DefaultFormatter::format(const LogEntry &entry) {
  std::ostringstream oss;

  for (size_t i = 0; i < pattern_.size(); ++i) {
    if (pattern_[i] == '%' && i + 1 < pattern_.size()) {
      switch (pattern_[i + 1]) {
      case 't':
        oss << formatTime(entry.timestamp);
        break;
      case 'l':
        oss << logLevelName(entry.level);
        break;
      case 'm':
        oss << entry.message;
        break;
      case 'f':
        oss << entry.file;
        break;
      case 'n':
        oss << entry.line;
        break;
      case 'F':
        oss << entry.function;
        break;
      case 'T':
        oss << entry.threadId;
        break;
      case '%':
        oss << '%';
        break;
      default:
        oss << pattern_[i] << pattern_[i + 1];
        break;
      }
      ++i;
    } else {
      oss << pattern_[i];
    }
  }

  return oss.str();
}

std::string
DefaultFormatter::formatTime(std::chrono::system_clock::time_point tp) {
  auto time = std::chrono::system_clock::to_time_t(tp);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                tp.time_since_epoch()) %
            1000;

  std::tm tm;
  localtime_r(&time, &tm);

  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << '.' << std::setfill('0')
      << std::setw(3) << ms.count();
  return oss.str();
}

std::string JsonFormatter::format(const LogEntry &entry) {
  std::ostringstream oss;
  oss << "{";
  oss << "\"timestamp\":\"" << formatTime(entry.timestamp) << "\",";
  oss << "\"level\":\"" << logLevelName(entry.level) << "\",";
  oss << "\"message\":\"" << escapeJson(entry.message) << "\"";
  if (!entry.file.empty()) {
    oss << ",\"file\":\"" << entry.file << "\"";
    oss << ",\"line\":" << entry.line;
  }
  if (!entry.function.empty()) {
    oss << ",\"function\":\"" << entry.function << "\"";
  }
  oss << ",\"thread\":\"" << entry.threadId << "\"";
  oss << "}";
  return oss.str();
}

std::string
JsonFormatter::formatTime(std::chrono::system_clock::time_point tp) {
  auto time = std::chrono::system_clock::to_time_t(tp);
  std::tm tm;
  localtime_r(&time, &tm);
  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
  return oss.str();
}

std::string JsonFormatter::escapeJson(const std::string &str) {
  std::string result;
  for (char c : str) {
    switch (c) {
    case '"':
      result += "\\\"";
      break;
    case '\\':
      result += "\\\\";
      break;
    case '\n':
      result += "\\n";
      break;
    case '\r':
      result += "\\r";
      break;
    case '\t':
      result += "\\t";
      break;
    default:
      result += c;
      break;
    }
  }
  return result;
}

std::string ColorFormatter::format(const LogEntry &entry) {
  std::string formatted = defaultFormatter_.format(entry);
  return colorize(entry.level, formatted);
}

std::string ColorFormatter::colorize(LogLevel level, const std::string &text) {
  const char *color;
  switch (level) {
  case LogLevel::Trace:
    color = "\033[90m";
    break; // 灰色
  case LogLevel::Debug:
    color = "\033[36m";
    break; // 青色
  case LogLevel::Info:
    color = "\033[32m";
    break; // 绿色
  case LogLevel::Warning:
    color = "\033[33m";
    break; // 黄色
  case LogLevel::Error:
    color = "\033[31m";
    break; // 红色
  case LogLevel::Fatal:
    color = "\033[35m";
    break; // 紫色
  default:
    color = "";
    break;
  }
  return std::string(color) + text + "\033[0m";
}

} // namespace log
} // namespace darwincore
