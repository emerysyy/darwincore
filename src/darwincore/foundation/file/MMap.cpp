//
// MMap.cpp
// DarwinCore
//

#include <darwincore/foundation/file/MMap.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace darwincore {
namespace file {

bool MMap::map(const std::string &path, MMapMode mode) {
  unmap();

  int openFlags = (mode == MMapMode::ReadOnly) ? O_RDONLY : O_RDWR;
  fd_ = ::open(path.c_str(), openFlags);
  if (fd_ == -1)
    return false;

  struct stat sb;
  if (fstat(fd_, &sb) == -1) {
    ::close(fd_);
    fd_ = -1;
    return false;
  }
  size_ = sb.st_size;

  int prot = PROT_READ;
  int flags = MAP_SHARED;

  switch (mode) {
  case MMapMode::ReadOnly:
    prot = PROT_READ;
    flags = MAP_PRIVATE;
    break;
  case MMapMode::ReadWrite:
    prot = PROT_READ | PROT_WRITE;
    flags = MAP_SHARED;
    break;
  case MMapMode::CopyOnWrite:
    prot = PROT_READ | PROT_WRITE;
    flags = MAP_PRIVATE;
    break;
  }

  data_ = ::mmap(nullptr, size_, prot, flags, fd_, 0);
  if (data_ == MAP_FAILED) {
    data_ = nullptr;
    ::close(fd_);
    fd_ = -1;
    return false;
  }

  mode_ = mode;
  return true;
}

void MMap::unmap() {
  if (data_) {
    ::munmap(data_, size_);
    data_ = nullptr;
  }
  if (fd_ != -1) {
    ::close(fd_);
    fd_ = -1;
  }
  size_ = 0;
}

bool MMap::sync(bool async) {
  if (!data_)
    return false;
  int flags = async ? MS_ASYNC : MS_SYNC;
  return ::msync(data_, size_, flags) == 0;
}

void MMap::prefetch() {
  if (data_) {
    ::madvise(data_, size_, MADV_WILLNEED);
  }
}

void MMap::setSequential() {
  if (data_) {
    ::madvise(data_, size_, MADV_SEQUENTIAL);
  }
}

void MMap::setRandom() {
  if (data_) {
    ::madvise(data_, size_, MADV_RANDOM);
  }
}

} // namespace file
} // namespace darwincore
