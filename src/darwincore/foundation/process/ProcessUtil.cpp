//
// Created by Darwin Core on 2023/9/8.
//

#include <darwincore/foundation/process/ProcessUtil.h>
#include <libproc.h>
#include <sys/sysctl.h>
#include <unistd.h>
#include <vector>
#include <csignal>
#include <cerrno>
#include <darwincore/foundation/string/StringUtils.h>

using namespace darwincore::process;
using namespace darwincore::string;

bool ProcessUtil::getProcessCPUUsage(pid_t pid, double &usage) {
    struct rusage_info_v2 rusage1{}, rusage2{};

    // 获取开始时间的资源使用情况
    if (proc_pid_rusage(pid, RUSAGE_INFO_V2, (void **)&rusage1) != 0) {
        return false;
    }

    int samplingInterval = 500;
    // 延时一段时间，例如 500ms
    usleep(samplingInterval * 1000);

    // 获取结束时间的资源使用情况
    if (proc_pid_rusage(pid, RUSAGE_INFO_V2, (void **)&rusage2) != 0) {
        return false;
    }

    // 计算 CPU 使用率
    uint64_t process_time = (rusage2.ri_system_time + rusage2.ri_user_time) - (rusage1.ri_system_time + rusage1.ri_user_time);
    double cpu_usage = 100.0 * process_time / (samplingInterval * 1000 * 1000);  // conv

    usage = cpu_usage;
    return true;
}

bool ProcessUtil::getProcessMemUsage(pid_t pid, uint64_t &usage) {
    struct proc_taskinfo info{};
    int ret = proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &info, sizeof(info));

    if (ret <= 0) {
        return false;
    }
    usage = info.pti_resident_size;
//    double mem_usage_mb = info.pti_resident_size / 1024.0 / 1024.0;
//    usage = mem_usage_mb;
    return true;
}

bool ProcessUtil::getProcessCommandline(pid_t pid, std::string &cmd) {
    /* Max size of arguments (KERN_ARGMAX) */
    int request_argmax[2] = {
            CTL_KERN, KERN_ARGMAX
    };

    int argmax = 0;
    size_t size = sizeof(argmax);
    int err = sysctl(request_argmax, 2, &argmax, &size, NULL, 0);
    if (err != KERN_SUCCESS) {
        return false;
    }

    /* Request arguments pointer */
    uint8_t *arguments = (uint8_t *)malloc(argmax);
    if (!arguments) {
        return false;
    }

    pid_t current_pid = pid;
    int request_args[3] = {
            CTL_KERN, KERN_PROCARGS2, current_pid
    };
    size = argmax;
    err = sysctl(request_args, 3, arguments, &size, NULL, 0);
    if (err != KERN_SUCCESS) {
        free(arguments);
        return false;
    }

    /*
     获取的 arguments buffer 不是utf8编码，如果直接使用 std::string
     以 utf-8 编码输出到文件，会出现奇怪到符号
     KERN_PROCARGS
     ${exec_path}\0\0\0\0\0\0\0\0${exec_path}\0${argv}

     KERN_PROCARGS2
     ${argc}\0\0\0${exec_path}\0\0\0\0\0\0\0\0${exec_path}\0${argv}
     */
    int argc = *arguments;
    uint8_t *arguments_ptr = arguments;
    // skip `argc`
    arguments += sizeof(argc);
    // skip `exec_path` which is a duplicate of argv[0]
    arguments += strlen((const char *)arguments);

    if (argc <= 0) {
        free(arguments_ptr);
        return false;
    }

    for (size_t i = 0; i < size;) {
        if ((*(arguments+i)) == '\0') {
            i++;
        }
        const char *arg = (const char *)(arguments+i);
        if (strlen(arg) > 0) {

            if (!cmd.empty()) {
                cmd.append(" ");
            }
            cmd.append(arg);

            i += strlen(arg);
        } else {
            i++;
        }
    }

    free(arguments_ptr);
    return true;
}

bool ProcessUtil::isProcessAlive(pid_t pid) {
    if (pid <= 0 || kill(pid, 0) == -1) {
        if (errno == ESRCH) {
            // 进程不存在
            return false;
        } else {
            // 其他错误，可能是权限问题等
            return false;
        }
    } else {
        // 如果没有错误，那么进程存在
        return true;
    }
}

bool ProcessUtil::getProcessBinPath(pid_t pid, std::string &path) {
    char pathBuff[PROC_PIDPATHINFO_MAXSIZE] = {0};
    if (proc_pidpath(pid, pathBuff, PROC_PIDPATHINFO_MAXSIZE) == 0) {
        path = std::string(pathBuff);
        return true;
    }

    return false;
}

bool ProcessUtil::getProcessName(pid_t pid, std::string &name) {

    char nameBuf[PROC_PIDPATHINFO_MAXSIZE] = {0};
    if (proc_name(pid, nameBuf, sizeof(nameBuf)) == 0) {
        name = std::string(nameBuf);
        return true;
    }

    return false;
}
