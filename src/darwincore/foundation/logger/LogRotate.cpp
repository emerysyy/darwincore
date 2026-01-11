//
// LogRotate.cpp
// DarwinCore
//

#include <darwincore/foundation/logger/LogRotate.h>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>

namespace darwincore {
namespace log {

using namespace darwincore::file;

LogRotate::LogRotate(const std::string &logPath, RotateConfig config)
    : logPath_(logPath), config_(std::move(config)), currentSize_(0) {
  FileManager fm;
  if (fm.isItemExistsAtPath(logPath_)) {
    struct stat st;
    if (stat(logPath_.c_str(), &st) == 0) {
      currentSize_ = st.st_size;
    }
  }
  lastRotateTime_ = std::chrono::system_clock::now();
}

bool LogRotate::shouldRotate() const {
  if (config_.policy == RotatePolicy::Never)
    return false;

  if (config_.policy == RotatePolicy::Size) {
    return currentSize_ >= config_.maxFileSize;
  }

  if (config_.policy == RotatePolicy::Daily) {
    return shouldRotateByTime(std::chrono::hours(24));
  }

  if (config_.policy == RotatePolicy::Hourly) {
    return shouldRotateByTime(std::chrono::hours(1));
  }

  return false;
}

bool LogRotate::rotate() {
  FileManager fm;
  if (!fm.isItemExistsAtPath(logPath_))
    return true;

  // 删除最老的文件
  for (int i = config_.maxFiles - 1; i >= 1; --i) {
    std::string from = getRotatedPath(i);
    std::string to = getRotatedPath(i + 1);
    if (fm.isItemExistsAtPath(from)) {
      if (i + 1 > config_.maxFiles) {
        fm.removeItemAtPath(from);
      } else {
        fm.moveItem(from, to, true);
      }
    }
  }

  // 重命名当前日志
  std::string rotatedPath = getRotatedPath(1);
  if (!fm.moveItem(logPath_, rotatedPath, true)) {
    return false;
  }

  // 压缩（可选）
  if (config_.compress) {
    compressFile(rotatedPath);
  }

  currentSize_ = 0;
  lastRotateTime_ = std::chrono::system_clock::now();
  return true;
}

void LogRotate::cleanup() {
  FileManager fm;
  for (int i = 1;; ++i) {
    std::string path = getRotatedPath(i);
    if (fm.isItemExistsAtPath(path)) {
      if (i > config_.maxFiles) {
        fm.removeItemAtPath(path);
      }
    } else {
      break;
    }
  }
}

std::string LogRotate::getRotatedPath(int index) const {
  return logPath_ + "." + std::to_string(index);
}

bool LogRotate::shouldRotateByTime(std::chrono::hours period) const {
  struct stat st;
  if (stat(logPath_.c_str(), &st) != 0) {
    return false;
  }

  auto lastWrite = std::chrono::system_clock::from_time_t(st.st_mtime);
  return std::chrono::system_clock::now() - lastWrite >= period;
}

void LogRotate::compressFile(const std::string &path) {
  // 简化实现：使用 gzip 命令
  std::string cmd = "gzip -f " + path;
  int res = std::system(cmd.c_str());
  (void)res;
}

} // namespace log
} // namespace darwincore
