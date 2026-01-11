//
// Created by Darwin Core on 2023/1/29.
//

#include <darwincore/foundation/file/FileLock.h>
#include <sys/file.h>
#include <unistd.h>

using namespace darwincore::file;

FileLock::FileLock() { _fd = -1; }

FileLock::~FileLock() { unlock(); }

FileLock::ErrorCode FileLock::lock(const std::string &path, bool sharedMode) {
  FileLock::ErrorCode code = Failure;
  do {

    if (_fd != -1) {
      code = Locked;
      break;
    }

    if (path.empty()) {
      code = Illegal;
      break;
    }

    int fd = open(path.c_str(), O_CREAT | O_RDWR, 0666);
    if (fd < 0) {
      code = CantOpen;
      break;
    }

    int lockType = sharedMode ? LOCK_SH : LOCK_EX;

    int rc = flock(fd, lockType | LOCK_NB);
    if (rc == 0) {
      code = Success;
      _fd = fd;
      break;
    }

    code = LockErr;
    if (errno == EWOULDBLOCK) {
      code = Blocked;
    }

  } while (false);

  return code;
}

void FileLock::unlock() {
  if (_fd == -1) {
    return;
  }
  flock(_fd, LOCK_UN);
  close(_fd);
  _fd = -1;
}
