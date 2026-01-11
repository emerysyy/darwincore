//
// Created by Darwin Core on 2023/1/28.
//

#ifndef MYLIB_FILEMANAGER_H
#define MYLIB_FILEMANAGER_H

#include <iostream>
#include <vector>

namespace darwincore {
namespace file {
// ========================================
// 文件管理器类
// ========================================
// 功能：
// 1. 提供文件和目录的创建、删除、移动、复制等操作
// 2. 支持遍历目录内容
// 3. 支持工作目录管理
// 4. 封装 POSIX 文件操作 API，提供更友好的接口
// ========================================
class FileManager {

public:
  // 获取指定路径下的所有子路径（包括文件和目录）
  std::vector<std::string> subpathsAtPath(const std::string &path);

  // 获取文件或目录的显示名称
  // hideExtension: 是否隐藏文件扩展名
  std::string displayNameAtPath(const std::string &path,
                                bool hideExtension = false);

  // 创建目录（带权限设置）
  // path: 目录路径
  // mode: 目录权限（如 0755）
  // createIntermediates: 是否创建中间目录（类似 mkdir -p）
  bool createDirectoryAtPath(const std::string &path, int mode,
                             bool createIntermediates = true);

  // 创建目录（使用默认权限）
  bool createDirectoryAtPath(const std::string &path,
                             bool createIntermediates = true);

  // 检查路径是否存在
  bool isItemExistsAtPath(const std::string &path);

  // 检查路径是否存在，并返回是否为目录
  bool isItemExistsAtPath(const std::string &path, bool &isDirectory);

  // 删除文件或目录（递归删除）
  bool removeItemAtPath(const std::string &path);

  // 移动文件或目录
  // fromPath: 源路径
  // toPath: 目标路径
  // cover: 如果目标已存在，是否覆盖
  bool moveItem(const std::string &fromPath, const std::string &toPath,
                bool cover = true);

  // 复制文件或目录
  // fromPath: 源路径
  // toPath: 目标路径
  // cover: 如果目标已存在，是否覆盖
  bool copyItem(const std::string &fromPath, const std::string &toPath,
                bool cover = true);

  // 重命名文件或目录
  bool renameItem(const std::string &fromPath, const std::string &toPath);

  // 清空文件内容（保留文件）
  bool cleanFile(const std::string &path);

  // 部分复制文件（按百分比）
  // percentage: 复制百分比（0.0 - 1.0）
  bool copyItem(const std::string &fromPath, std::string toPath,
                double percentage);

  // 获取当前工作目录
  bool getCurrentWorkSpace(std::string &workSpace);

  // 修改当前工作目录
  bool changeCurrentWorkSpace(std::string &workSpace);
};
} // namespace file
} // namespace darwincore

#endif // MYLIB_FILEMANAGER_H
