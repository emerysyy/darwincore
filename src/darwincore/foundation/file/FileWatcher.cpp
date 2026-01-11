//
// FileWatcher.cpp
// DarwinCore
//

#include <darwincore/foundation/file/FileWatcher.h>
#include <mutex>
#include <vector>

namespace darwincore {
namespace file {

void FileWatcher::addPath(const std::string &path) {
  std::lock_guard<std::mutex> lock(mutex_);
  paths_.push_back(path);
}

void FileWatcher::setCallback(FileWatchCallback callback) {
  callback_ = std::move(callback);
}

#ifdef __APPLE__
void FileWatcher::fsEventCallback(ConstFSEventStreamRef, void *context,
                                  size_t numEvents, void *eventPaths,
                                  const FSEventStreamEventFlags eventFlags[],
                                  const FSEventStreamEventId[]) {
  auto *self = static_cast<FileWatcher *>(context);
  CFArrayRef pathArray = static_cast<CFArrayRef>(eventPaths);

  std::vector<FileChange> changes;
  for (size_t i = 0; i < numEvents; ++i) {
    CFStringRef cfPath =
        static_cast<CFStringRef>(CFArrayGetValueAtIndex(pathArray, i));
    char path[1024];
    CFStringGetCString(cfPath, path, sizeof(path), kCFStringEncodingUTF8);

    FileEvent event = FileEvent::Unknown;
    if (eventFlags[i] & kFSEventStreamEventFlagItemCreated)
      event = FileEvent::Created;
    else if (eventFlags[i] & kFSEventStreamEventFlagItemRemoved)
      event = FileEvent::Deleted;
    else if (eventFlags[i] & kFSEventStreamEventFlagItemRenamed)
      event = FileEvent::Renamed;
    else if (eventFlags[i] & kFSEventStreamEventFlagItemModified)
      event = FileEvent::Modified;

    changes.push_back({path, event});
  }

  if (self->callback_ && !changes.empty())
    self->callback_(changes);
}
#endif

bool FileWatcher::start() {
#ifdef __APPLE__
  if (running_ || paths_.empty())
    return false;
  running_ = true;

  watchThread_ = std::thread([this]() {
    CFMutableArrayRef pathsToWatch =
        CFArrayCreateMutable(nullptr, paths_.size(), &kCFTypeArrayCallBacks);
    for (const auto &path : paths_) {
      CFStringRef cfPath = CFStringCreateWithCString(nullptr, path.c_str(),
                                                     kCFStringEncodingUTF8);
      CFArrayAppendValue(pathsToWatch, cfPath);
      CFRelease(cfPath);
    }

    FSEventStreamContext context = {0, this, nullptr, nullptr, nullptr};
    stream_ =
        FSEventStreamCreate(nullptr, &FileWatcher::fsEventCallback, &context,
                            pathsToWatch, kFSEventStreamEventIdSinceNow,
                            0.5, // 延迟500ms合并事件
                            kFSEventStreamCreateFlagFileEvents |
                                kFSEventStreamCreateFlagUseCFTypes);
    CFRelease(pathsToWatch);

    if (!stream_) {
      running_ = false;
      return;
    }

    runLoop_ = CFRunLoopGetCurrent();
    FSEventStreamScheduleWithRunLoop(stream_, runLoop_, kCFRunLoopDefaultMode);
    FSEventStreamStart(stream_);
    CFRunLoopRun();
  });
  return true;
#else
  return false;
#endif
}

void FileWatcher::stop() {
#ifdef __APPLE__
  if (!running_)
    return;
  running_ = false;

  if (stream_) {
    FSEventStreamStop(stream_);
    FSEventStreamInvalidate(stream_);
    FSEventStreamRelease(stream_);
    stream_ = nullptr;
  }

  if (runLoop_) {
    CFRunLoopStop(runLoop_);
    runLoop_ = nullptr;
  }

  if (watchThread_.joinable())
    watchThread_.join();
#endif
}

} // namespace file
} // namespace darwincore
