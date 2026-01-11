//
// Created by 李培 on 2026/1/6.
//

#include <darwincore/foundation/process/ProcessTree.h>

using namespace darwincore::process;

#include <sstream>
#include <iostream>
#include <algorithm>
#include <unordered_set>

// ---------------- ProcKey ----------------
bool ProcKey::operator==(const ProcKey& other) const {
    return pid == other.pid && pidversion == other.pidversion;
}

size_t std::hash<ProcKey>::operator()(const ProcKey& k) const {
    return std::hash<pid_t>()(k.pid) ^ (std::hash<uint32_t>()(k.pidversion) << 1);
}

// ---------------- ProcessTree ----------------

void ProcessTree::addProcess(const ProcessInfo& info) {
    std::unique_lock lock(mutex_);
    ProcKey key{info.pid, info.pidversion};
    if (nodes_.count(key)) return;

    auto node = std::make_shared<ProcessNode>();
    node->info = info;

    ProcKey pkey{info.ppid, info.ppidversion};
    auto pit = nodes_.find(pkey);
    if (pit != nodes_.end()) {
        node->parent = pit->second;
        pit->second->children.push_back(node);

        auto pcur = pit->second;
        while (auto parent = pcur) {
            ++pcur->alive_descendant_count;
            pcur = pcur->parent.lock();
        }
    }

    nodes_[key] = node;
}

std::shared_ptr<ProcessTree::ProcessNode> ProcessTree::getResponsibleNode(const std::shared_ptr<ProcessNode>& node) const {
    ProcKey rpkey{node->info.rpid, node->info.rpidversion};
    auto it = nodes_.find(rpkey);
    return it != nodes_.end() ? it->second : nullptr;
}

bool ProcessTree::getProcessInfo(const ProcKey& key, ProcessInfo& out) const {
    std::shared_lock lock(mutex_);
    auto it = nodes_.find(key);
    if (it == nodes_.end()) return false;
    out = it->second->info;
    return true;
}

void ProcessTree::markProcessDead(const ProcKey& key) {
    std::unique_lock lock(mutex_);
    auto it = nodes_.find(key);
    if (it == nodes_.end()) return;

    auto node = it->second;
    if (node->state == ProcState::Dead) return;

    node->state = ProcState::Dead;

    // 更新父节点 alive_descendant_count
    auto pcur = node->parent.lock();
    while (pcur) {
        if (pcur->alive_descendant_count > 0) --pcur->alive_descendant_count;
        pcur = pcur->parent.lock();
    }

    // 清理节点
    std::function<void(std::shared_ptr<ProcessNode>)> tryCleanup = [&](std::shared_ptr<ProcessNode> n) {
        if (n->state == ProcState::Dead && n->alive_descendant_count == 0) {
            if (auto p = n->parent.lock()) {
                p->children.remove(n);
                tryCleanup(p);
            }
            nodes_.erase(n->key());
        }
    };

    tryCleanup(node);
}

std::vector<ProcessInfo> ProcessTree::getProcLineage(const ProcKey& key) const {
    std::vector<ProcessInfo> result;
    std::unordered_set<ProcKey> visited;

    std::shared_lock lock(mutex_);
    auto it = nodes_.find(key);
    if (it == nodes_.end()) return result;

    auto node = it->second;

    // 父链
    auto cur = node;
    while (cur && cur->info.pid > 1) {
        if (!visited.insert(cur->key()).second) break;
        result.push_back(cur->info);
        cur = cur->parent.lock();
    }

    // responsible 链
    cur = node;
    while (auto rnode = getResponsibleNode(cur)) {
        if (!visited.insert(rnode->key()).second) break;
        result.push_back(rnode->info);
        cur = rnode;
    }

    // 自身加入
    if (visited.insert(node->key()).second) {
        result.push_back(node->info);
    }

    return result;
}

void ProcessTree::printProcessTree(const ProcKey& rootKey) const {
    std::cout << debugProcessTree(rootKey);
}

std::string ProcessTree::debugProcessTree(const ProcKey& rootKey) const {
    std::shared_lock lock(mutex_);
    auto it = nodes_.find(rootKey);
    if (it == nodes_.end()) return "Root not found\n";

    std::ostringstream oss;
    debugProcessTreeHelper(it->second, 0, oss);
    return oss.str();
}

void ProcessTree::debugProcessTreeHelper(const std::shared_ptr<ProcessNode>& node, int depth, std::ostringstream& oss) const {
    if (!node) return;

    std::string indent(depth * 2, ' ');
    oss << indent << "|-- PID=" << node->info.pid
        << " Name=" << node->info.proc_name
        << " State=" << (node->state == ProcState::Alive ? "Alive" : "Dead")
        << " AliveDescendants=" << node->alive_descendant_count
        << " PPID=" << node->info.ppid
        << " PPIDVersion=" << node->info.ppidversion
        << " RPID=" << node->info.rpid
        << " RPIDVersion=" << node->info.rpidversion
        << "\n";

    for (const auto& child : node->children) {
        debugProcessTreeHelper(child, depth + 1, oss);
    }
}
