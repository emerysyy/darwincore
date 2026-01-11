//
// DirectoryIterator.h
// DarwinCore
//

#ifndef DARWINCORE_DIRECTORY_ITERATOR_H
#define DARWINCORE_DIRECTORY_ITERATOR_H

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace darwincore {
namespace file {

namespace fs = std::filesystem;

/// 目录条目类型
enum class EntryType { File, Directory, Symlink, Other };

/// 目录条目
struct DirectoryEntry {
  std::string path;
  std::string name;
  EntryType type;
  size_t size;         // 仅对文件有效
  std::time_t modTime; // 修改时间
};

/// 目录遍历过滤器
using EntryFilter = std::function<bool(const DirectoryEntry &)>;

/// 目录遍历器
class DirectoryIterator {
public:
  explicit DirectoryIterator(const std::string &path) : rootPath_(path) {}

  /// 设置过滤器
  DirectoryIterator &filter(EntryFilter f) {
    filter_ = std::move(f);
    return *this;
  }

  /// 设置是否递归
  DirectoryIterator &recursive(bool r) {
    recursive_ = r;
    return *this;
  }

  /// 设置是否跟随符号链接
  DirectoryIterator &followSymlinks(bool f) {
    followSymlinks_ = f;
    return *this;
  }

  /// 设置最大递归深度
  DirectoryIterator &maxDepth(int depth) {
    maxDepth_ = depth;
    return *this;
  }

  /// 获取所有条目
  std::vector<DirectoryEntry> entries() const;

  /// 仅获取文件
  std::vector<std::string> files() const;

  /// 仅获取目录
  std::vector<std::string> directories() const;

  /// 按扩展名过滤
  std::vector<std::string> filesWithExtension(const std::string &ext) const;

  /// 遍历回调
  void forEach(std::function<void(const DirectoryEntry &)> callback) const;

  /// 计算总大小
  size_t totalSize() const;

  /// 计算文件数量
  size_t fileCount() const;

private:
  void iterate(std::function<void(const DirectoryEntry &)> callback) const;

  void processEntry(
      const fs::directory_entry &entry,
      const std::function<void(const DirectoryEntry &)> &callback) const;

  std::string rootPath_;
  EntryFilter filter_;
  bool recursive_ = false;
  bool followSymlinks_ = false;
  int maxDepth_ = -1;
};

} // namespace file
} // namespace darwincore

#endif // DARWINCORE_DIRECTORY_ITERATOR_H
