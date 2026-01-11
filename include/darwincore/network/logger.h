//
// DarwinCore Network 模块
// 网络日志系统
//
// 功能说明：
//   提供网络模块的日志记录功能，支持不同级别的日志输出。
//   支持自定义日志回调函数，可将日志输出到控制台、文件或其他地方。
//
// 特性：
//   - 支持多种日志级别（Trace、Debug、Info、Warning、Error、Fatal）
//   - 日志格式：[时间] [级别] [文件:行] 日志内容
//   - 全局日志级别控制
//   - 可设置自定义日志回调函数
//   - 线程安全
//
// 作者: DarwinCore Network 团队
// 日期: 2026

#ifndef DARWINCORE_NETWORK_LOGGER_H
#define DARWINCORE_NETWORK_LOGGER_H

#include <string>
#include <functional>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <iostream>

namespace darwincore {
namespace network {

/**
 * @brief 日志级别枚举
 */
enum class LogLevel {
  kTrace = 0,    ///< 最详细的跟踪信息
  kDebug = 1,    ///< 调试信息
  kInfo = 2,     ///< 一般信息
  kWarning = 3,  ///< 警告信息
  kError = 4,    ///< 错误信息
  kFatal = 5     ///< 致命错误信息
};

/**
 * @brief 日志回调函数类型
 * @param level 日志级别
 * @param message 格式化后的日志消息
 * @param file 源文件名
 * @param line 源代码行号
 */
using LogCallback = std::function<void(LogLevel level,
                                        const std::string& message,
                                        const char* file,
                                        int line)>;

/**
 * @brief 网络日志系统
 *
 * 提供网络模块的日志记录功能。
 * 这是一个单例类，全局唯一。
 *
 * 使用示例：
 *   @code
 *   // 设置日志回调
 *   NetworkLogger::Instance().SetLogCallback([](LogLevel level, const std::string& msg, const char* file, int line) {
 *     std::cout << msg << std::endl;
 *   });
 *
 *   // 设置日志级别
 *   NetworkLogger::Instance().SetLogLevel(LogLevel::kDebug);
 *
 *   // 记录日志
 *   NW_LOG_INFO("服务器启动成功，端口=" << port);
 *   NW_LOG_ERROR("连接失败: " << error_msg);
 *   @endcode
 */
class NetworkLogger {
public:
  /**
   * @brief 获取单例实例
   */
  static NetworkLogger& Instance() {
    static NetworkLogger instance;
    return instance;
  }

  // 删除拷贝和移动
  NetworkLogger(const NetworkLogger&) = delete;
  NetworkLogger& operator=(const NetworkLogger&) = delete;

  /**
   * @brief 设置日志回调函数
   * @param callback 日志回调函数
   *
   * 如果不设置回调，默认输出到控制台。
   */
  void SetLogCallback(LogCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    callback_ = std::move(callback);
  }

  /**
   * @brief 设置日志级别
   * @param level 日志级别
   *
   * 只有大于等于此级别的日志才会被输出。
   */
  void SetLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    log_level_ = level;
  }

  /**
   * @brief 获取当前日志级别
   */
  LogLevel GetLogLevel() const {
    return log_level_;
  }

  /**
   * @brief 记录日志（内部方法）
   * @param level 日志级别
   * @param message 日志消息
   * @param file 源文件名
   * @param line 源代码行号
   */
  void Log(LogLevel level, const std::string& message, const char* file, int line) {
    if (level < log_level_) {
      return;
    }

    std::string formatted = FormatMessage(level, message, file, line);

    std::lock_guard<std::mutex> lock(mutex_);
    if (callback_) {
      callback_(level, formatted, file, line);
    } else {
      // 默认输出到控制台
      DefaultOutput(level, formatted);
    }
  }

private:
  NetworkLogger() : log_level_(LogLevel::kInfo), callback_(nullptr) {}

  /**
   * @brief 格式化日志消息
   */
  std::string FormatMessage(LogLevel level, const std::string& message,
                             const char* file, int line) {
    // 获取当前时间
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << "[" << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S")
       << "." << std::setfill('0') << std::setw(3) << ms.count() << "] "
       << "[" << LevelToString(level) << "] "
       << "[" << GetFileName(file) << ":" << line << "] "
       << message;

    return ss.str();
  }

  /**
   * @brief 将日志级别转换为字符串
   */
  const char* LevelToString(LogLevel level) {
    switch (level) {
      case LogLevel::kTrace:   return "TRACE";
      case LogLevel::kDebug:   return "DEBUG";
      case LogLevel::kInfo:    return "INFO ";
      case LogLevel::kWarning: return "WARN ";
      case LogLevel::kError:   return "ERROR";
      case LogLevel::kFatal:   return "FATAL";
      default:                 return "UNKNOWN";
    }
  }

  /**
   * @brief 从完整路径中提取文件名
   */
  const char* GetFileName(const char* path) {
    const char* last_slash = path;
    while (*path) {
      if (*path == '/' || *path == '\\') {
        last_slash = path + 1;
      }
      path++;
    }
    return last_slash;
  }

  /**
   * @brief 默认日志输出（输出到控制台）
   */
  void DefaultOutput(LogLevel level, const std::string& message) {
    switch (level) {
      case LogLevel::kTrace:
      case LogLevel::kDebug:
      case LogLevel::kInfo:
        std::cout << message << std::endl;
        break;
      case LogLevel::kWarning:
      case LogLevel::kError:
      case LogLevel::kFatal:
        std::cerr << message << std::endl;
        break;
    }
  }

  LogLevel log_level_;              ///< 当前日志级别
  LogCallback callback_;            ///< 自定义日志回调
  std::mutex mutex_;                ///< 互斥锁，保证线程安全
};

// ==================== 便捷宏定义 ====================

/**
 * @brief 记录 Trace 级别日志
 */
#define NW_LOG_TRACE(stream) \
  darwincore::network::NetworkLogger::Instance().Log( \
    darwincore::network::LogLevel::kTrace, \
    [&]() -> std::string { std::stringstream ss; ss << stream; return ss.str(); }(), \
    __FILE__, __LINE__)

/**
 * @brief 记录 Debug 级别日志
 */
#define NW_LOG_DEBUG(stream) \
  darwincore::network::NetworkLogger::Instance().Log( \
    darwincore::network::LogLevel::kDebug, \
    [&]() -> std::string { std::stringstream ss; ss << stream; return ss.str(); }(), \
    __FILE__, __LINE__)

/**
 * @brief 记录 Info 级别日志
 */
#define NW_LOG_INFO(stream) \
  darwincore::network::NetworkLogger::Instance().Log( \
    darwincore::network::LogLevel::kInfo, \
    [&]() -> std::string { std::stringstream ss; ss << stream; return ss.str(); }(), \
    __FILE__, __LINE__)

/**
 * @brief 记录 Warning 级别日志
 */
#define NW_LOG_WARNING(stream) \
  darwincore::network::NetworkLogger::Instance().Log( \
    darwincore::network::LogLevel::kWarning, \
    [&]() -> std::string { std::stringstream ss; ss << stream; return ss.str(); }(), \
    __FILE__, __LINE__)

/**
 * @brief 记录 Error 级别日志
 */
#define NW_LOG_ERROR(stream) \
  darwincore::network::NetworkLogger::Instance().Log( \
    darwincore::network::LogLevel::kError, \
    [&]() -> std::string { std::stringstream ss; ss << stream; return ss.str(); }(), \
    __FILE__, __LINE__)

/**
 * @brief 记录 Fatal 级别日志
 */
#define NW_LOG_FATAL(stream) \
  darwincore::network::NetworkLogger::Instance().Log( \
    darwincore::network::LogLevel::kFatal, \
    [&]() -> std::string { std::stringstream ss; ss << stream; return ss.str(); }(), \
    __FILE__, __LINE__)

}  // namespace network
}  // namespace darwincore

#endif  // DARWINCORE_NETWORK_LOGGER_H
