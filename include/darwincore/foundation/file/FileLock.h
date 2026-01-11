//
// Created by Darwin Core on 2023/1/29.
//

#ifndef MYLIB_FILELOCK_H
#define MYLIB_FILELOCK_H

#include <iostream>
#include <vector>

namespace darwincore {
namespace file {
// ========================================
// 文件锁类
// ========================================
// 功能：
// 1. 提供基于文件的进程间互斥锁机制
// 2. 支持共享锁（读锁）和独占锁（写锁）
// 3. 使用 POSIX fcntl 实现
// ========================================
class FileLock {
private:
  int _fd; // 文件描述符

public:
  // 错误码枚举
  enum ErrorCode {
    Failure = -1, // 未知失败
    Success = 0,  // 成功
    Locked = 1,   // 文件已被锁定
    Illegal = 2,  // 非法参数
    CantOpen = 3, // 无法打开文件
    LockErr = 4,  // 锁定操作失败
    Blocked = 5   // 操作被阻塞
  };

public:
  FileLock();
  ~FileLock();

  // 对指定文件加锁
  // path: 文件路径
  // sharedMode: 是否使用共享锁模式（true=共享锁/读锁，false=独占锁/写锁）
  // 返回：错误码，Success 表示成功
  ErrorCode lock(const std::string &path, bool sharedMode);

  // 解锁当前文件
  void unlock();
};
} // namespace file
} // namespace darwincore

#endif // MYLIB_FILELOCK_H
