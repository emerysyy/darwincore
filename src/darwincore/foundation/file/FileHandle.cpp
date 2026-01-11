//
// Created by Darwin Core on 2023/9/12.
//

#include <darwincore/foundation/file/FileHandle.h>

using namespace darwincore::file;

FileHandle::FileHandle(const std::string &path) : _path(path) {}

FileHandle::~FileHandle() { close(); }

bool FileHandle::open(FileHandle::Mode mode) {
  if (isOpen()) {
    return _mode == mode;
  }
  _mode = mode;

  std::ios_base::openmode openMode = std::ios::in;
  if (mode == Write) {
    openMode = std::ios::out | std::ios::trunc;
  } else if (mode == Append) {
    openMode = std::ios::out | std::ios::app;
  }
  _fstream.open(_path, openMode);

  return isOpen();
}

void FileHandle::close() {
  if (isOpen()) {
    _fstream.flush();
    _fstream.close();
  }
}

bool FileHandle::isOpen() { return _fstream.is_open(); }

FileSize FileHandle::size() {
  bool needclose = false;
  if (!isOpen()) {
    if (!open(ReadOnly)) {
      return 0;
    }
    needclose = true;
  }

  std::streampos currentPosition = _fstream.tellg();
  _fstream.seekg(0, std::ios::end);
  FileSize fileSize = static_cast<FileSize>(_fstream.tellg());
  _fstream.seekg(currentPosition, std::ios::beg);

  if (needclose) {
    close();
  }

  return fileSize;
}

bool FileHandle::read(FileSize offset, FileSize len, std::string &content) {
  if (isOpen()) {
    if (offset >= size()) {
      return false;
    }

    _fstream.seekg(offset, std::ios::beg);
    char *buffer = new char[len];
    _fstream.read(buffer, len);
    content.assign(buffer, len);

    delete[] buffer;
    return true;
  } else {
    return false;
  }
}

void FileHandle::write(const std::string &content) {
  if (isOpen()) {
    _fstream << content;
    _fstream.flush();
  } else {
  }
}

void FileHandle::clear() {
  bool reopen = false;
  if (isOpen()) {
    reopen = true;
    _fstream.close();
  }

  std::ifstream ifs;
  ifs.open(_path, std::ios::out | std::ios::trunc);
  if (ifs.is_open()) {
    ifs.close();
  }

  if (reopen) {
    open(_mode);
  }
}
