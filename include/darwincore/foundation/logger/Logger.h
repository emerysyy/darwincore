//
// Created by Darwin Core on 2023/9/11.
//

#ifndef MYLIB_LOGGER_H
#define MYLIB_LOGGER_H

#include <cstdarg>
#include <iostream>
#include <memory>
#include <mutex>
#include <os/log.h>
#include <queue>
#include <string>
#include <thread>
#include <utility>

namespace darwincore {
namespace file {
class FileHandle; // 前向声明 FileHandle 类
}
} // namespace darwincore

namespace darwincore {
namespace log {

// ========================================
// 日志级别枚举
// ========================================
// 从低到高依次为：TRACE -> DEBUG -> INFO -> WARNING -> ERROR -> FATAL -> OFF
// 设置的级别越高，输出的日志越少（低于当前级别的日志不会被输出）
// ========================================
typedef enum {
  TRACE = 0,   // 最详细的日志级别，用于详细的调试信息
  DEBUG = 1,   // 调试信息，用于开发调试
  INFO = 2,    // 一般信息，记录程序运行状态
  WARNING = 3, // 警告信息，表示潜在问题
  ERROR = 4,   // 错误信息，表示发生了错误
  FATAL = 5,   // 致命错误，表示程序无法继续运行
  OFF = 6      // 关闭所有日志输出
} Level;

// ========================================
// 日志类
// ========================================
// 功能：
// 1. 支持多级别日志输出
// 2. 支持多种输出方式：文件、控制台、系统日志（syslog）
// 3. 支持日志文件自动滚动（按大小）
// 4. 异步日志写入，避免阻塞主线程
// ========================================
class Logger {
private:
  // ========================================
  // 日志对象内部类
  // ========================================
  // 封装单条日志的所有信息
  // ========================================
  class LogObj {
  public:
    LogObj(Level level, std::string timestamp, std::string log);

    ~LogObj();

  public:
    Level _level;           // 日志级别
    std::string _timestamp; // 时间戳
    std::string _log;       // 日志内容
  };

public:
  // 构造函数
  // path: 日志文件路径（如 "/var/log/myapp.log"）
  // consoleEnable: 是否启用控制台输出（true=输出到控制台，false=不输出）
  // syslogEnable: 是否启用系统日志输出（true=输出到系统日志，false=不输出）
  // maxLogFileSize: 日志文件最大大小（字节），超过后会自动滚动并创建新文件（如
  // 10MB = 10 * 1024 * 1024） rollbackCount: 保留的历史日志文件数量（如 3
  // 表示保留 .log, .log.1, .log.2）
  Logger(std::string path, bool consoleEnable, bool syslogEnable,
         unsigned long long maxLogFileSize, int rollbackCount);

  // 析构函数
  // 说明：会停止日志线程并刷新所有待处理的日志
  ~Logger();

  // 设置日志级别
  // level: 日志级别，低于此级别的日志将不会被输出
  // 说明：例如设置为 INFO，则 TRACE 和 DEBUG 级别的日志将被过滤
  void setLevel(Level level);

  // 获取当前日志级别
  // 返回：当前的日志级别
  Level getLevel();

  // 输出日志（可变参数函数，类似 printf）
  // level: 日志级别
  // fmt: 格式化字符串（支持 printf 风格的格式化，如 "Error: %s, code: %d"）
  // ...: 可变参数列表，用于格式化字符串
  void log(Level level, const char *fmt, ...);

public:
  // 获取日志级别的字符串描述
  // level: 日志级别
  // 返回：日志级别的字符串表示（如 "INFO", "ERROR"）
  static std::string levelDescription(Level level);

private:
  // 日志线程主函数：异步处理日志写入
  void run();

  // 打开日志文件（force: 强制重新打开）
  // force:
  // 是否强制重新打开文件（true=先关闭再打开，false=只在文件未打开时打开）
  void openLogFile(bool force);

  // 写入日志到文件
  // obj: 日志对象
  void write(const LogObj &obj);

  // 写入日志到系统日志
  // obj: 日志对象
  void syslog(const LogObj &obj);

  // 写入日志到控制台
  // obj: 日志对象
  void console(const LogObj &obj);

private:
  bool _running;                   // 运行标志
  Level _level;                    // 当前日志级别
  bool _consoleEnable;             // 是否启用控制台输出
  bool _syslogEnable;              // 是否启用系统日志输出
  std::string _logFile;            // 日志文件路径
  unsigned long long _currentSize; // 当前日志文件大小（字节）
  unsigned long long _maxFileSize; // 日志文件最大大小（字节）
  int _rollbackCount;              // 历史日志文件数量
  os_log_t _osLog;                 // macOS 系统日志句柄
  std::unique_ptr<darwincore::file::FileHandle>
      _fileHandle;          // 文件句柄（用于写入日志文件）
  std::mutex _logMutex;     // 日志队列互斥锁
  std::queue<LogObj> _logs; // 待处理的日志队列
  std::thread _logThread;   // 异步日志写入线程
  std::condition_variable _logConditionVar; // 条件变量，用于通知日志线程
};
}; // namespace log
} // namespace darwincore

#endif // MYLIB_LOGGER_H
