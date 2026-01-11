//
// PathUtils.h
// DarwinCore
//

#ifndef DARWINCORE_PATH_UTILS_H
#define DARWINCORE_PATH_UTILS_H

#include <filesystem>
#include <string>
#include <vector>

namespace darwincore {
namespace file {

namespace fs = std::filesystem;

/// 路径工具类
class PathUtils {
public:
  /// 路径拼接
  static std::string join(const std::string &base, const std::string &path);

  template <typename... Args>
  static std::string join(const std::string &first, const std::string &second,
                          Args... rest) {
    return join(join(first, second), rest...);
  }

  /// 获取文件名（带扩展名）
  static std::string fileName(const std::string &path);

  /// 获取文件名（不带扩展名）
  static std::string baseName(const std::string &path);

  /// 获取扩展名（带点）
  static std::string extension(const std::string &path);

  /// 获取扩展名（不带点）
  static std::string extensionWithoutDot(const std::string &path);

  /// 获取父目录
  static std::string parentPath(const std::string &path);

  /// 规范化路径
  static std::string normalize(const std::string &path);

  /// 获取绝对路径
  static std::string absolutePath(const std::string &path);

  /// 获取相对路径
  static std::string relativePath(const std::string &path,
                                  const std::string &base);

  /// 检查路径是否存在
  static bool exists(const std::string &path);

  /// 检查是否是绝对路径
  static bool isAbsolute(const std::string &path);

  /// 检查是否是相对路径
  static bool isRelative(const std::string &path);

  /// 检查是否是目录
  static bool isDirectory(const std::string &path);

  /// 检查是否是文件
  static bool isFile(const std::string &path);

  /// 检查是否是符号链接
  static bool isSymlink(const std::string &path);

  /// 更改扩展名
  static std::string changeExtension(const std::string &path,
                                     const std::string &newExt);

  /// 添加后缀（在扩展名之前）
  static std::string addSuffix(const std::string &path,
                               const std::string &suffix);

  /// 分割路径为组件
  static std::vector<std::string> components(const std::string &path);

  /// 获取根路径
  static std::string rootPath(const std::string &path);

  /// 展开波浪号（用户目录）
  static std::string expandTilde(const std::string &path);

  /// 获取用户主目录
  static std::string homeDirectory();

  /// 获取当前工作目录
  static std::string currentDirectory();

  /// 创建目录（包括父目录）
  static bool createDirectories(const std::string &path);

  /// 获取唯一文件名（如果存在则添加数字后缀）
  static std::string uniquePath(const std::string &path);
};

} // namespace file
} // namespace darwincore

#endif // DARWINCORE_PATH_UTILS_H
