//
// LogSink.cpp
// DarwinCore
//

#include <darwincore/foundation/logger/LogSink.h>

namespace darwincore {
namespace log {

void ConsoleSink::write(const LogEntry &entry) {
  if (!shouldLog(entry.level))
    return;

  std::lock_guard<std::mutex> lock(mutex_);
  std::string formatted = formatter_->format(entry);

  if (entry.level >= LogLevel::Error) {
    std::cerr << formatted << std::endl;
  } else {
    std::cout << formatted << std::endl;
  }
}

void ConsoleSink::flush() {
  std::cout.flush();
  std::cerr.flush();
}
FileSink::FileSink(const std::string &path, bool append) : path_(path) {
  auto mode = append ? std::ios::app : std::ios::trunc;
  file_.open(path, std::ios::out | mode);
}

FileSink::FileSink(const std::string &path, RotateConfig rotateConfig)
    : path_(path), rotator_(new LogRotate(path, std::move(rotateConfig))) {
  file_.open(path, std::ios::out | std::ios::app);
}

FileSink::~FileSink() {
  if (file_.is_open()) {
    file_.close();
  }
}

void FileSink::write(const LogEntry &entry) {
  if (!shouldLog(entry.level))
    return;

  std::lock_guard<std::mutex> lock(mutex_);

  // 检查轮转
  if (rotator_ && rotator_->shouldRotate()) {
    file_.close();
    rotator_->rotate();
    file_.open(path_, std::ios::out | std::ios::app);
  }

  std::string formatted = formatter_->format(entry);
  file_ << formatted << std::endl;

  if (rotator_) {
    rotator_->recordWrite(formatted.size() + 1);
  }
}

void FileSink::flush() {
  std::lock_guard<std::mutex> lock(mutex_);
  file_.flush();
}

#ifdef __APPLE__
void OSLogSink::write(const LogEntry &entry) {
  if (!shouldLog(entry.level))
    return;

  os_log_type_t type;
  switch (entry.level) {
  case LogLevel::Trace:
  case LogLevel::Debug:
    type = OS_LOG_TYPE_DEBUG;
    break;
  case LogLevel::Info:
    type = OS_LOG_TYPE_INFO;
    break;
  case LogLevel::Warning:
    type = OS_LOG_TYPE_DEFAULT;
    break;
  case LogLevel::Error:
    type = OS_LOG_TYPE_ERROR;
    break;
  case LogLevel::Fatal:
    type = OS_LOG_TYPE_FAULT;
    break;
  default:
    type = OS_LOG_TYPE_DEFAULT;
  }

  os_log_with_type(osLog_, type, "%{public}s", entry.message.c_str());
}

void OSLogSink::flush() {
  // OSLog 不需要显式刷新
}
#endif

void MultiSink::write(const LogEntry &entry) {
  for (auto &sink : sinks_) {
    sink->write(entry);
  }
}

void MultiSink::flush() {
  for (auto &sink : sinks_) {
    sink->flush();
  }
}

void CallbackSink::write(const LogEntry &entry) {
  if (!this->shouldLog(entry.level))
    return;
  if (callback_)
    callback_(entry);
}

} // namespace log
} // namespace darwincore
