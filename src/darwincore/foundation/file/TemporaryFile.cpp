//
// TemporaryFile.cpp
// DarwinCore
//

#include <darwincore/foundation/file/TemporaryFile.h>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace darwincore {
namespace file {

TemporaryFile::TemporaryFile(const std::string &prefix,
                             const std::string &suffix)
    : autoDelete_(true) {
  std::string pattern = "/tmp/" + prefix + "XXXXXX" + suffix;
  std::vector<char> pathTemplate(pattern.begin(), pattern.end());
  pathTemplate.push_back('\0');

  if (suffix.empty()) {
    fd_ = mkstemp(pathTemplate.data());
  } else {
    fd_ = mkstemps(pathTemplate.data(), static_cast<int>(suffix.size()));
  }

  if (fd_ != -1) {
    path_ = pathTemplate.data();
  }
}

TemporaryFile::TemporaryFile(const std::string &path, bool autoDelete)
    : path_(path), fd_(-1), autoDelete_(autoDelete) {}

TemporaryFile::~TemporaryFile() {
  close();
  if (autoDelete_ && !path_.empty()) {
    std::remove(path_.c_str());
  }
}

TemporaryFile::TemporaryFile(TemporaryFile &&other) noexcept
    : path_(std::move(other.path_)), fd_(other.fd_),
      autoDelete_(other.autoDelete_) {
  other.fd_ = -1;
  other.path_.clear();
}

TemporaryFile &TemporaryFile::operator=(TemporaryFile &&other) noexcept {
  if (this != &other) {
    close();
    if (autoDelete_ && !path_.empty())
      std::remove(path_.c_str());

    path_ = std::move(other.path_);
    fd_ = other.fd_;
    autoDelete_ = other.autoDelete_;
    other.fd_ = -1;
    other.path_.clear();
  }
  return *this;
}

bool TemporaryFile::write(const void *data, size_t size) {
  if (fd_ == -1)
    return false;
  return ::write(fd_, data, size) == static_cast<ssize_t>(size);
}

bool TemporaryFile::write(const std::string &data) {
  return write(data.data(), data.size());
}

void TemporaryFile::close() {
  if (fd_ != -1) {
    ::close(fd_);
    fd_ = -1;
  }
}

std::string TemporaryFile::release() {
  autoDelete_ = false;
  std::string result = std::move(path_);
  path_.clear();
  return result;
}

std::string TemporaryFile::tempDirectory() {
  const char *tmpdir = std::getenv("TMPDIR");
  return tmpdir ? tmpdir : "/tmp";
}

// TemporaryDirectory 实现

TemporaryDirectory::TemporaryDirectory(const std::string &prefix)
    : autoDelete_(true) {
  std::string pattern = "/tmp/" + prefix + "XXXXXX";
  std::vector<char> pathTemplate(pattern.begin(), pattern.end());
  pathTemplate.push_back('\0');

  if (mkdtemp(pathTemplate.data())) {
    path_ = pathTemplate.data();
  }
}

TemporaryDirectory::~TemporaryDirectory() {
  if (autoDelete_ && !path_.empty()) {
    removeRecursive(path_);
  }
}

std::string TemporaryDirectory::release() {
  autoDelete_ = false;
  return std::move(path_);
}

void TemporaryDirectory::removeRecursive(const std::string &path) {
  // 简化实现，实际应使用 filesystem 或递归删除
  std::system(("rm -rf " + path).c_str());
}

} // namespace file
} // namespace darwincore
