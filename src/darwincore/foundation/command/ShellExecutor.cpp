//
// Created by Darwin Core on 2023/1/17.
//

#include <darwincore/foundation/command/ShellExecutor.h>
#include <sstream>
#include <iostream>

using namespace darwincore::command;

int ShellExecutor::execute(const std::string &cmd, std::string &ret) {
    return _execute(cmd, ret);
}

int ShellExecutor::execute(const std::string &cmd) {
    return _execute(cmd);
}

int ShellExecutor::_execute(const std::string &cmd, std::string &ret) {
    int code = -1;
    if (cmd.empty()) {
        return code;
    }
    FILE *fp = popen(cmd.c_str(), "r");
    if (fp != NULL) {
        std::stringstream ss;
        char buf[1024] = { 0 };
        while (!feof(fp)) {
            bzero(buf, sizeof(buf));
            if (fgets(buf, sizeof(buf), fp) == NULL) {
                continue;
            }
            ss << buf;
        }
        code = pclose(fp);

        if (code != 127 && code != -1)
        {
            // shell正常退出
            if(WIFEXITED(code))
            {
                code = WEXITSTATUS(code);
            }
                // shell异常退出
            else if(WIFSIGNALED(code))
            {
                code = WTERMSIG(code);
            }
        }
        ret = ss.str();
    }

    return code;
}

int ShellExecutor::_execute(const std::string &cmd) {
    int code = -1;
    if (cmd.empty()) {
        return code;
    }

    code = system(cmd.c_str());

    return code;
}