//
// Created by Darwin Core on 2023/9/8.
//

#ifndef MYLIB_PROCESSUTIL_H
#define MYLIB_PROCESSUTIL_H

#include <iostream>
#include <string>

namespace darwincore {
  namespace process {
      // ========================================
      // 进程工具类
      // ========================================
      // 功能：
      // 提供进程相关的查询工具函数
      // ========================================
      class ProcessUtil {
      public:
          // 检查进程是否存活
          // pid: 进程 ID
          // 返回：true 表示进程存活，false 表示进程不存在
          static bool isProcessAlive(pid_t pid);

          // 获取进程 CPU 使用率
          // pid: 进程 ID
          // usage: 输出参数，存储 CPU 使用率（百分比，0-100）
          // 返回：true 表示获取成功，false 表示失败
          static bool getProcessCPUUsage(pid_t pid, double &usage);

          // 获取进程内存使用量
          // pid: 进程 ID
          // usage: 输出参数，存储内存使用量（字节）
          // 返回：true 表示获取成功，false 表示失败
          static bool getProcessMemUsage(pid_t pid, uint64_t &usage);

          // 获取进程命令行参数
          // pid: 进程 ID
          // cmd: 输出参数，存储完整的命令行字符串
          // 返回：true 表示获取成功，false 表示失败
          static bool getProcessCommandline(pid_t pid, std::string &cmd);

          // 获取进程可执行文件路径
          // pid: 进程 ID
          // path: 输出参数，存储可执行文件的完整路径
          // 返回：true 表示获取成功，false 表示失败
          static bool getProcessBinPath(pid_t pid, std::string &path);

          // 获取进程名称
          // pid: 进程 ID
          // name: 输出参数，存储进程名称（不包含路径）
          // 返回：true 表示获取成功，false 表示失败
          static bool getProcessName(pid_t pid, std::string &name);
      };

  };
};





#endif //MYLIB_PROCESSUTIL_H
