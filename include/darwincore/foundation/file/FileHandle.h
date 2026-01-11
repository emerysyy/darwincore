//
// Created by Darwin Core on 2023/9/12.
//

#ifndef MYLIB_FILEHANDLE_H
#define MYLIB_FILEHANDLE_H

#include <fstream>
#include <iostream>
#include <string>

namespace darwincore {
namespace file {

// 文件大小类型定义
using FileSize = unsigned long long;

// ========================================
// 文件句柄类
// ========================================
// 功能：
// 1. 提供文件的打开、关闭、读写操作
// 2. 支持随机访问读写
// 3. 提供文件大小查询功能
// ========================================
class FileHandle {

public:
  // 文件打开模式枚举
  enum Mode {
    ReadOnly = 0, // 只读模式
    Write = 1,    // 写入模式（覆盖）
    Append = 2    // 追加模式
  };

public:
  // 构造函数
  explicit FileHandle(const std::string &path);
  ~FileHandle();

  // 打开文件
  bool open(Mode mode);

  // 关闭文件
  void close();

  // 检查文件是否已打开
  bool isOpen();

  // 获取文件大小
  FileSize size();

  // 读取文件内容
  // offset: 起始偏移量
  // len: 读取长度
  // content: 输出参数，存储读取的内容
  bool read(FileSize offset, FileSize len, std::string &content);

  // 写入文件内容
  void write(const std::string &content);

  // 清空文件内容
  void clear();

private:
  std::string _path;     // 文件路径
  Mode _mode;            // 打开模式
  std::fstream _fstream; // 文件流对象
};
} // namespace file
} // namespace darwincore

#endif // MYLIB_FILEHANDLE_H
