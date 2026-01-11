//
// SecureDelete.cpp
// DarwinCore
//

#include <darwincore/foundation/file/SecureDelete.h>
#include <darwincore/foundation/file/DirectoryIterator.h>
#include <algorithm>
#include <vector>

namespace darwincore {
namespace file {

bool SecureDelete::deleteFile(const std::string &path, SecureDeleteMode mode) {
  int fd = ::open(path.c_str(), O_WRONLY);
  if (fd == -1)
    return false;

  struct stat st;
  if (fstat(fd, &st) == -1) {
    ::close(fd);
    return false;
  }

  size_t fileSize = st.st_size;
  std::vector<uint8_t> buffer(std::min(fileSize, size_t(64 * 1024)));

  bool success = true;
  switch (mode) {
  case SecureDeleteMode::Simple:
    success = overwriteZeros(fd, fileSize, buffer);
    break;
  case SecureDeleteMode::DoD:
    success = overwriteDoD(fd, fileSize, buffer);
    break;
  case SecureDeleteMode::Gutmann:
    success = overwriteGutmann(fd, fileSize, buffer);
    break;
  }

  // 同步到磁盘
  fsync(fd);
  ::close(fd);

  if (!success)
    return false;

  // 重命名后删除（隐藏原始文件名）
  std::string randomName = generateRandomName(path);
  std::rename(path.c_str(), randomName.c_str());

  return ::remove(randomName.c_str()) == 0;
}

bool SecureDelete::deleteDirectory(const std::string &path,
                                   SecureDeleteMode mode) {
  // 首先获取所有文件和目录（按深度优先）
  darwincore::file::DirectoryIterator iter(path);
  iter.recursive(true);

  // 收集所有文件和子目录
  std::vector<std::string> files;
  std::vector<std::string> directories;
  iter.forEach([&path, &files, &directories](const DirectoryEntry &e) {
    if (e.type == EntryType::File) {
      files.push_back(e.path);
    } else if (e.type == EntryType::Directory && e.path != path) {
      directories.push_back(e.path);
    }
  });

  // 安全删除所有文件
  for (const auto &file : files) {
    if (!deleteFile(file, mode)) {
      return false;
    }
  }

  // 删除所有子目录（从最深到最浅）
  std::sort(directories.begin(), directories.end(),
            [](const std::string &a, const std::string &b) {
              return a.length() > b.length();
            });
  for (const auto &dir : directories) {
    if (::rmdir(dir.c_str()) != 0) {
      return false;
    }
  }

  // 最后删除根目录
  return ::rmdir(path.c_str()) == 0;
}

void SecureDelete::secureZeroMemory(void *ptr, size_t size) {
  volatile uint8_t *p = static_cast<volatile uint8_t *>(ptr);
  while (size--)
    *p++ = 0;
}

bool SecureDelete::overwriteZeros(int fd, size_t size,
                                  std::vector<uint8_t> &buffer) {
  std::memset(buffer.data(), 0x00, buffer.size());
  return overwritePattern(fd, size, buffer);
}

bool SecureDelete::overwriteDoD(int fd, size_t size,
                                std::vector<uint8_t> &buffer) {
  // Pass 1: 写入0x00
  std::memset(buffer.data(), 0x00, buffer.size());
  if (!overwritePattern(fd, size, buffer))
    return false;

  // Pass 2: 写入0xFF
  std::memset(buffer.data(), 0xFF, buffer.size());
  if (!overwritePattern(fd, size, buffer))
    return false;

  // Pass 3: 写入随机数据
  return overwriteRandom(fd, size, buffer);
}

bool SecureDelete::overwriteGutmann(int fd, size_t size,
                                    std::vector<uint8_t> &buffer) {
  static const uint8_t patterns[][3] = {
      {0x55, 0x55, 0x55}, {0xAA, 0xAA, 0xAA}, {0x92, 0x49, 0x24},
      {0x49, 0x24, 0x92}, {0x24, 0x92, 0x49}, {0x00, 0x00, 0x00},
      {0x11, 0x11, 0x11}, {0x22, 0x22, 0x22}, {0x33, 0x33, 0x33},
      {0x44, 0x44, 0x44}, {0x55, 0x55, 0x55}, {0x66, 0x66, 0x66},
      {0x77, 0x77, 0x77}, {0x88, 0x88, 0x88}, {0x99, 0x99, 0x99},
      {0xAA, 0xAA, 0xAA}, {0xBB, 0xBB, 0xBB}, {0xCC, 0xCC, 0xCC},
      {0xDD, 0xDD, 0xDD}, {0xEE, 0xEE, 0xEE}, {0xFF, 0xFF, 0xFF}};

  for (int i = 0; i < 4; ++i) {
    if (!overwriteRandom(fd, size, buffer))
      return false;
  }

  for (const auto &pattern : patterns) {
    for (size_t i = 0; i < buffer.size(); i += 3) {
      buffer[i] = pattern[0];
      if (i + 1 < buffer.size())
        buffer[i + 1] = pattern[1];
      if (i + 2 < buffer.size())
        buffer[i + 2] = pattern[2];
    }
    if (!overwritePattern(fd, size, buffer))
      return false;
  }

  for (int i = 0; i < 4; ++i) {
    if (!overwriteRandom(fd, size, buffer))
      return false;
  }

  return true;
}

bool SecureDelete::overwritePattern(int fd, size_t size,
                                    const std::vector<uint8_t> &buffer) {
  lseek(fd, 0, SEEK_SET);
  size_t remaining = size;
  while (remaining > 0) {
    size_t toWrite = std::min(remaining, buffer.size());
    if (::write(fd, buffer.data(), toWrite) != static_cast<ssize_t>(toWrite)) {
      return false;
    }
    remaining -= toWrite;
  }
  return true;
}

bool SecureDelete::overwriteRandom(int fd, size_t size,
                                   std::vector<uint8_t> &buffer) {
  thread_local std::random_device rd;
  thread_local std::mt19937 gen(rd());
  std::uniform_int_distribution<uint16_t> dist(0, 255);

  lseek(fd, 0, SEEK_SET);
  size_t remaining = size;
  while (remaining > 0) {
    for (auto &byte : buffer)
      byte = static_cast<uint8_t>(dist(gen));
    size_t toWrite = std::min(remaining, buffer.size());
    if (::write(fd, buffer.data(), toWrite) != static_cast<ssize_t>(toWrite)) {
      return false;
    }
    remaining -= toWrite;
  }
  return true;
}

std::string SecureDelete::generateRandomName(const std::string &path) {
  // 简化实现，实际应从 path 解析
  std::string dir = "";
  auto pos = path.find_last_of("/");
  if (pos != std::string::npos) {
    dir = path.substr(0, pos);
  }

  static std::random_device rd;
  static std::mt19937 gen(rd());
  std::uniform_int_distribution<> dist(0, 15);

  std::string name;
  for (int i = 0; i < 16; ++i) {
    name += "0123456789abcdef"[dist(gen)];
  }

  return dir.empty() ? name : (dir + "/" + name);
}

} // namespace file
} // namespace darwincore
