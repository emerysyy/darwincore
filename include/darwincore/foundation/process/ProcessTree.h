//
// Created by 李培 on 2026/1/6.
//

#ifndef PROCESSTREE_H
#define PROCESSTREE_H

#include <unordered_map>
#include <memory>
#include <list>
#include <vector>
#include <string>
#include <shared_mutex>

namespace darwincore {
    namespace process {
        // ========================================
        // 进程键结构
        // ========================================
        // 用于唯一标识一个进程（PID + 版本号）
        // ========================================
        struct ProcKey {
            pid_t pid;          // 进程 ID
            uint32_t pidversion;  // 进程版本号（用于区分 PID 重复使用的情况）

            bool operator==(const ProcKey& other) const;
        };
    };
};

// 为 ProcKey 提供 std::hash 特化，使其可用于 unordered_map
namespace std {
    template<>
    struct hash<darwincore::process::ProcKey> {
        size_t operator()(const darwincore::process::ProcKey& k) const;
    };
}

namespace darwincore {
    namespace process {

        // ========================================
        // 进程状态枚举
        // ========================================
        enum class ProcState {
            Alive,  // 存活
            Dead    // 已死亡
        };

        // ========================================
        // 进程信息结构
        // ========================================
        struct ProcessInfo {
            pid_t pid;              // 进程 ID
            uint32_t pidversion;    // 进程版本号
            pid_t ppid;             // 父进程 ID
            uint32_t ppidversion;   // 父进程版本号
            pid_t rpid;             // 真实进程 ID（用于调试）
            uint32_t rpidversion;   // 真实进程版本号
            std::string proc_name;  // 进程名称
            std::string proc_path;  // 进程路径
        };

        // ========================================
        // 进程树类
        // ========================================
        // 功能：
        // 1. 维护系统的进程树结构
        // 2. 支持添加、删除进程节点
        // 3. 支持查询进程信息和进程血缘关系
        // 4. 支持打印进程树（用于调试）
        // ========================================
        class ProcessTree {
        public:
            // 添加进程节点
            void addProcess(const ProcessInfo& info);

            // 标记进程为已死亡
            void markProcessDead(const ProcKey& key);

            // 获取进程信息
            bool getProcessInfo(const ProcKey& key, ProcessInfo& out) const;

            // 获取进程血缘链（从当前进程到根进程）
            std::vector<ProcessInfo> getProcLineage(const ProcKey& key) const;

            // 打印进程树（用于调试）
            void printProcessTree(const ProcKey& rootKey = {1,1}) const;

            // 获取进程树的调试字符串
            std::string debugProcessTree(const ProcKey& rootKey = {1,1}) const;

        private:
            // ========================================
            // 进程节点结构（内部使用）
            // ========================================
            struct ProcessNode {
                ProcessInfo info;                                      // 进程信息
                std::weak_ptr<ProcessNode> parent;                     // 父节点（弱引用，避免循环引用）
                std::list<std::shared_ptr<ProcessNode>> children;     // 子节点列表
                ProcState state { ProcState::Alive };                   // 进程状态
                size_t alive_descendant_count { 0 };                   // 存活子进程数量

                // 获取进程键
                ProcKey key() const { return {info.pid, info.pidversion}; }
            };

            mutable std::shared_mutex mutex_;                          // 读写锁，保护 nodes_ 的访问
            std::unordered_map<ProcKey, std::shared_ptr<ProcessNode>> nodes_;  // 进程节点映射

            // 获取负责的节点（用于清理已死亡节点）
            std::shared_ptr<ProcessNode> getResponsibleNode(const std::shared_ptr<ProcessNode>& node) const;

            // 打印进程树的辅助函数（递归）
            void debugProcessTreeHelper(const std::shared_ptr<ProcessNode>& node, int depth, std::ostringstream& oss) const;
        };

    };
};


#endif //PROCESSTREE_H
