//
// Created by Darwin Core on 2023/9/11.
//

#include <darwincore/foundation/logger/Logger.h>
#include <darwincore/foundation/date/DateTime.h>
#include <darwincore/foundation/file/FileHandle.h>
#include <darwincore/foundation/file/FileManager.h>
#include <darwincore/foundation/file/FilePath.h>
#include <darwincore/foundation/process/ProcessUtil.h>
#include <darwincore/foundation/string/StringUtils.h>
#include <darwincore/foundation/thread/AsyncQueue.h>
#include <sstream>
#include <vector>

using namespace darwincore::log;
using namespace darwincore::string;
using namespace darwincore::date;
using namespace darwincore::thread;
using namespace darwincore::file;
using namespace darwincore::process;

Logger::LogObj::LogObj(Level level, std::string timestamp, std::string log)
    : _level(level), _timestamp(std::move(timestamp)), _log(std::move(log)) {}

Logger::LogObj::~LogObj() {}

Logger::Logger(std::string path, bool consoleEnable, bool syslogEnable,
               unsigned long long maxLogFileSize, int rollbackCount)
    : _running(true), _level(INFO), _consoleEnable(consoleEnable),
      _syslogEnable(syslogEnable), _logFile(path), _currentSize(0),
      _maxFileSize(maxLogFileSize), _rollbackCount(rollbackCount), _osLog(NULL),
      _logThread(&Logger::run, this) {
  if (syslogEnable) {
    pid_t pid = getpid();
    std::string procName;
    ProcessUtil::getProcessName(pid, procName);
    _osLog = os_log_create("darwincore.foundation.logger", procName.c_str());
  }
  openLogFile(false);
}

Logger::~Logger() {
  _running = false;
  _fileHandle->close();
}

std::string Logger::levelDescription(Level level) {
  switch (level) {
  case TRACE:
    return "TRACE";
  case DEBUG:
    return "DEBUG";
  case INFO:
    return "INFO";
  case WARNING:
    return "WARNING";
  case ERROR:
    return "ERROR";
  case FATAL:
    return "FATAL";
  case OFF:
    return "OFF";
  }
}

void Logger::setLevel(Level level) { _level = level; }

Level Logger::getLevel() { return _level; }

void Logger::log(Level level, const char *fmt, ...) {
  if (level < this->getLevel()) {
    return;
  }
  DateTime date = DateTime::now();

  va_list args;
  va_start(args, fmt);
  std::string logStr = StringUtils::stringWithFormat(fmt, args);
  va_end(args);

  std::stringstream ss;
  std::thread::id threadId = std::this_thread::get_id();
  ss << threadId;

  std::string timeStr = date.format("%Y-%m-%d %H:%M:%S.%f");
  std::string levelStr = levelDescription(level);
  std::string tidStr = "[" + ss.str() + "]";

  logStr = tidStr + " " + levelStr + " " + logStr + "\n";

  AsyncQueue::queue().submit([this, level, timeStr, logStr]() {
    std::unique_lock<std::mutex> lock(_logMutex);
    this->_logs.emplace(level, timeStr, logStr);
    this->_logConditionVar.notify_one();
  });
}

void Logger::run() {

  while (_running) {
    std::unique_lock<std::mutex> lock(_logMutex);
    _logConditionVar.wait(
        lock, [this]() { return !this->_running || !this->_logs.empty(); });

    if (!this->_logs.empty()) {
      LogObj obj = this->_logs.front();
      this->_logs.pop();

      if (obj._level >= this->getLevel()) {

        console(obj);

        syslog(obj);

        write(obj);
      }
    }
  }
}

void Logger::write(const Logger::LogObj &obj) {
  //    std::cout << obj._log << std::endl;
  if (!_running) {
    return;
  }

  std::string log = obj._timestamp + " " + obj._log;
  _fileHandle->write(log);
  _currentSize += log.size();

  if (_currentSize >= _maxFileSize) {
    _fileHandle->close();
    openLogFile(true);
  }
}

void Logger::openLogFile(bool force) {

  FileManager fm;
  std::string filePath = _logFile;
  int rollbackCount = _rollbackCount;

  FilePath fp(filePath);
  FilePath parentDir = fp.parent();
  std::string name = fp.name();
  std::string ext = fp.extensionName();

  bool rollback = force;
  if (!force && fm.isItemExistsAtPath(filePath)) {
    FileHandle f(filePath);
    if (f.size() >= _maxFileSize) {
      rollback = true;
    }
  }

  if (rollback) {
    for (int i = rollbackCount - 1; i >= 0; i--) {
      std::string from;
      std::string to;

      if (i == 0) {
        from = parentDir.append(name + "." + ext);
      } else {
        from = parentDir.append(name + "." + std::to_string(i) + "." + ext);
      }
      to = parentDir.append(name + "." + std::to_string(i + 1) + "." + ext);

      if (fm.isItemExistsAtPath(from)) {
        fm.moveItem(from, to, true);
      }
    }
  }

  _fileHandle = std::unique_ptr<FileHandle>(new FileHandle(_logFile));
  if (!_fileHandle->open(file::FileHandle::Append)) {
    perror("open log file error");
    throw std::runtime_error("can't open log file");
  }
  _currentSize = _fileHandle->size();
}

void Logger::syslog(const Logger::LogObj &obj) {
  if (_syslogEnable && _osLog) {
    const char *content = obj._log.c_str();
    switch (obj._level) {
    case TRACE:
      os_log_debug(_osLog, "%s", content);
      break;
    case DEBUG:
      os_log_debug(_osLog, "%s", content);
      break;
    case INFO:
      os_log_info(_osLog, "%s", content);
      break;
    case WARNING:
      os_log_info(_osLog, "%s", content);
      break;
    case ERROR:
      os_log_error(_osLog, "%s", content);
      break;
    case FATAL:
      os_log_error(_osLog, "%s", content);
      break;
    case OFF:
      break;
    }
  }
}

void Logger::console(const Logger::LogObj &obj) {
  if (_consoleEnable) {
    std::cout << obj._timestamp << " " << obj._log;
  }
}
