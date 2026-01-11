//
// Created by Darwin Core on 2023/1/28.
//

#ifndef MYLIB_FILEPATH_H
#define MYLIB_FILEPATH_H

#include <iostream>
#include <vector>

namespace darwincore {
namespace file {
// ========================================
// 文件路径类
// ========================================
// 功能：
// 1. 提供文件路径的解析和操作
// 2. 支持路径的拼接和规范化
// 3. 提供路径组件的访问（父目录、文件名、扩展名等）
// ========================================
class FilePath {

private:
  std::vector<std::string> mPathComponents; // 路径组件数组
private:
  // 私有构造函数，用于内部创建路径
  FilePath(std::vector<std::string> components);

  // 从字符串中提取路径组件
  void extractPathComponents(const std::string &path);

  // 将路径组件组装成字符串
  std::string assemblyPath(const std::vector<std::string> &components);

public:
  // 构造函数
  explicit FilePath(const std::string &path);
  ~FilePath() = default;

  // 转换为字符串
  std::string toString();

  // 获取文件扩展名（包含点号）
  std::string extensionName();

  // 获取显示名称（带扩展名）
  std::string displayName();

  // 获取文件名（不带路径）
  std::string name();

  // 获取父路径（返回 FilePath 对象）
  FilePath parent();

  // 获取父目录路径（返回字符串）
  std::string parentDir();

  // 拼接路径组件（返回字符串）
  std::string append(const std::string &name);

  // 拼接路径组件（返回 FilePath 对象）
  FilePath appendNode(const std::string &name);
};
} // namespace file
} // namespace darwincore

#endif // MYLIB_FILEPATH_H
