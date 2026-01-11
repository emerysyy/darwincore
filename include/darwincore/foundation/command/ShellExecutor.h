//
// Created by Darwin Core on 2023/1/17.
//

#ifndef MYLIB_SHELLEXECUTOR_H
#define MYLIB_SHELLEXECUTOR_H

#include <string>

namespace darwincore {
    namespace command {
        // ========================================
        // Shell 命令执行器类
        // ========================================
        // 功能：
        // 1. 提供执行 Shell 命令的静态方法
        // 2. 支持获取命令执行结果
        // 3. 封装 POSIX popen 函数，简化 Shell 命令调用
        // ========================================
        class ShellExecutor {
        public:
            // 执行 Shell 命令并获取输出
            // cmd: 要执行的 Shell 命令字符串
            // ret: 输出参数，存储命令执行的标准输出内容
            // 返回：命令的退出状态码，0 表示成功，非 0 表示失败
            static int execute(const std::string &cmd, std::string &ret);

            // 执行 Shell 命令（不获取输出）
            // cmd: 要执行的 Shell 命令字符串
            // 返回：命令的退出状态码，0 表示成功，非 0 表示失败
            static int execute(const std::string &cmd);

        private:
            // 内部实现：执行命令并获取输出
            static int _execute(const std::string &cmd, std::string &ret);

            // 内部实现：执行命令（不获取输出）
            static int _execute(const std::string &cmd);
        };
    };
};





#endif //MYLIB_SHELLEXECUTOR_H
