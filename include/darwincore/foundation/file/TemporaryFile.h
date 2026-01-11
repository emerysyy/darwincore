//
// TemporaryFile.h
// DarwinCore
//

#ifndef DARWINCORE_TEMPORARY_FILE_H
#define DARWINCORE_TEMPORARY_FILE_H

#include <cstdio>
#include <fstream>
#include <string>
#include <unistd.h>
#include <vector>

namespace darwincore {
namespace file {

/// 临时文件 RAII 管理
class TemporaryFile {
public:
  /// 创建临时文件
  explicit TemporaryFile(const std::string &prefix = "tmp",
                         const std::string &suffix = "");

  /// 从现有文件创建（不自动删除）
  explicit TemporaryFile(const std::string &path, bool autoDelete);

  ~TemporaryFile();

  TemporaryFile(const TemporaryFile &) = delete;
  TemporaryFile &operator=(const TemporaryFile &) = delete;

  TemporaryFile(TemporaryFile &&other) noexcept;
  TemporaryFile &operator=(TemporaryFile &&other) noexcept;

  /// 获取路径
  [[nodiscard]] const std::string &path() const { return path_; }

  /// 获取文件描述符
  [[nodiscard]] int fd() const { return fd_; }

  /// 是否有效
  [[nodiscard]] bool isValid() const { return !path_.empty(); }

  /// 写入数据
  bool write(const void *data, size_t size);

  bool write(const std::string &data);

  /// 关闭文件
  void close();

  /// 保留文件（不自动删除）
  std::string release();

  /// 设置是否自动删除
  void setAutoDelete(bool autoDelete) { autoDelete_ = autoDelete; }

  /// 获取临时目录
  static std::string tempDirectory();

private:
  std::string path_;
  int fd_ = -1;
  bool autoDelete_ = true;
};

/// 临时目录 RAII 管理
class TemporaryDirectory {
public:
  explicit TemporaryDirectory(const std::string &prefix = "tmpdir");

  ~TemporaryDirectory();

  TemporaryDirectory(const TemporaryDirectory &) = delete;
  TemporaryDirectory &operator=(const TemporaryDirectory &) = delete;

  [[nodiscard]] const std::string &path() const { return path_; }
  [[nodiscard]] bool isValid() const { return !path_.empty(); }

  std::string release();

private:
  static void removeRecursive(const std::string &path);

  std::string path_;
  bool autoDelete_;
};

} // namespace file
} // namespace darwincore

#endif // DARWINCORE_TEMPORARY_FILE_H
