//
// FileWatcher.h
// DarwinCore
//

#ifndef DARWINCORE_FILE_WATCHER_H
#define DARWINCORE_FILE_WATCHER_H

#include <atomic>
#include <functional>
#include <mutex>
#include <os/log.h>
#include <string>
#include <thread>
#include <vector>

#ifdef __APPLE__
#include <CoreServices/CoreServices.h>
#endif

namespace darwincore {
namespace file {

/// 文件变化事件类型
enum class FileEvent { Created, Modified, Deleted, Renamed, Unknown };

/// 文件变化信息
struct FileChange {
  std::string path;
  FileEvent event;
};

/// 文件监控回调
using FileWatchCallback = std::function<void(const std::vector<FileChange> &)>;

/// 文件/目录监控器（基于 FSEvents）
class FileWatcher {
public:
  FileWatcher() : running_(false), stream_(nullptr) {}

  ~FileWatcher() { stop(); }

  FileWatcher(const FileWatcher &) = delete;
  FileWatcher &operator=(const FileWatcher &) = delete;

  /// 添加监控路径
  void addPath(const std::string &path);

  /// 设置回调
  void setCallback(FileWatchCallback callback);

  /// 开始监控
  bool start();

  /// 停止监控
  void stop();

  bool isRunning() const { return running_; }

private:
#ifdef __APPLE__
  static void fsEventCallback(ConstFSEventStreamRef, void *context,
                              size_t numEvents, void *eventPaths,
                              const FSEventStreamEventFlags eventFlags[],
                              const FSEventStreamEventId[]);

  FSEventStreamRef stream_;
  CFRunLoopRef runLoop_ = nullptr;
#endif

  std::vector<std::string> paths_;
  FileWatchCallback callback_;
  std::atomic<bool> running_;
  std::thread watchThread_;
  std::mutex mutex_;
};

} // namespace file
} // namespace darwincore

#endif // DARWINCORE_FILE_WATCHER_H
