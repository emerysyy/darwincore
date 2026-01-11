//
// LogRotate.h
// DarwinCore
//

#ifndef DARWINCORE_LOG_ROTATE_H
#define DARWINCORE_LOG_ROTATE_H

#include <darwincore/foundation/file/FileManager.h>
#include <chrono>
#include <functional>
#include <string>

namespace darwincore {
namespace log {

/// 日志轮转策略
enum class RotatePolicy {
  Size,   // 按大小轮转
  Daily,  // 每日轮转
  Hourly, // 每小时轮转
  Never   // 不轮转
};

/// 日志轮转配置
struct RotateConfig {
  RotatePolicy policy = RotatePolicy::Size;
  size_t maxFileSize = 10 * 1024 * 1024; // 10MB
  int maxFiles = 5;                      // 保留文件数
  bool compress = false;                 // 是否压缩旧日志
};

/// 日志轮转器
class LogRotate {
public:
  explicit LogRotate(const std::string &logPath, RotateConfig config = {});

  /// 检查是否需要轮转
  bool shouldRotate() const;

  /// 执行轮转
  bool rotate();

  /// 记录写入大小
  void recordWrite(size_t bytes) { currentSize_ += bytes; }

  /// 获取当前文件大小
  size_t currentSize() const { return currentSize_; }

  /// 获取配置
  const RotateConfig &config() const { return config_; }

  /// 修改配置
  void setConfig(RotateConfig config) { config_ = std::move(config); }

  /// 清理旧日志
  void cleanup();

private:
  std::string getRotatedPath(int index) const;

  bool shouldRotateByTime(std::chrono::hours period) const;

  void compressFile(const std::string &path);

  std::string logPath_;
  RotateConfig config_;
  size_t currentSize_;
  std::chrono::system_clock::time_point lastRotateTime_;
};

} // namespace log
} // namespace darwincore

#endif // DARWINCORE_LOG_ROTATE_H
