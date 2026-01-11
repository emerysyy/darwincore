//
// ConcurrentMap.h
// DarwinCore
//

#ifndef DARWINCORE_CONCURRENT_MAP_H
#define DARWINCORE_CONCURRENT_MAP_H

#include <functional>
#include <optional>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

#include <mutex>
#include <shared_mutex>
#include <unordered_map>

namespace darwincore {
namespace container {

/**
 * @brief 并发哈希映射 (Concurrent Map)
 *
 * 实用开发建议与使用场景：
 * 1. 全局配置/状态库：存储系统运行期间的全局变更项，支持高频读取和低频更新。
 * 2. 内存缓存：作为分布式缓存（如 Redis）的本地一级缓存（L1 Cache）。
 * 3. 用户Session/连接管理：在网关或代理中，用于维护活跃客户端的映射关系。
 *
 * 开发建议：
 * - ConcurrentMap (读写锁)：适用于读多写少的场景，读操作可以完全并发。
 * - ShardedMap (分片模型)：通过将数据分散到多个子
 * Map，极大降低了写写冲突，适用于极高并发的写密集型场景。
 *
 * @tparam Key 键类型
 * @tparam Value 值类型
 */
template <typename Key, typename Value, typename Hash = std::hash<Key>>
class ConcurrentMap {
public:
  ConcurrentMap() = default;
  ~ConcurrentMap() = default;

  ConcurrentMap(const ConcurrentMap &) = delete;
  ConcurrentMap &operator=(const ConcurrentMap &) = delete;

  void insert(const Key &key, const Value &value) {
    std::unique_lock lock(mutex_);
    map_[key] = value;
  }

  void insert(const Key &key, Value &&value) {
    std::unique_lock lock(mutex_);
    map_[key] = std::move(value);
  }

  std::optional<Value> get(const Key &key) const {
    std::shared_lock lock(mutex_);
    auto it = map_.find(key);
    if (it == map_.end())
      return std::nullopt;
    return it->second;
  }

  bool contains(const Key &key) const {
    std::shared_lock lock(mutex_);
    return map_.find(key) != map_.end();
  }

  bool erase(const Key &key) {
    std::unique_lock lock(mutex_);
    return map_.erase(key) > 0;
  }

  void clear() {
    std::unique_lock lock(mutex_);
    map_.clear();
  }

  size_t size() const {
    std::shared_lock lock(mutex_);
    return map_.size();
  }

  bool empty() const {
    std::shared_lock lock(mutex_);
    return map_.empty();
  }

  /// 获取或插入（原子操作）
  template <typename Factory>
  Value getOrInsert(const Key &key, Factory &&factory) {
    {
      std::shared_lock lock(mutex_);
      auto it = map_.find(key);
      if (it != map_.end())
        return it->second;
    }
    std::unique_lock lock(mutex_);
    auto it = map_.find(key);
    if (it != map_.end())
      return it->second;
    auto [inserted, _] = map_.emplace(key, factory());
    return inserted->second;
  }

  /// 遍历所有元素
  template <typename Func> void forEach(Func &&func) const {
    std::shared_lock lock(mutex_);
    for (const auto &[k, v] : map_)
      func(k, v);
  }

  /// 获取所有键
  std::vector<Key> keys() const {
    std::shared_lock lock(mutex_);
    std::vector<Key> result;
    result.reserve(map_.size());
    for (const auto &[k, _] : map_)
      result.push_back(k);
    return result;
  }

private:
  mutable std::shared_mutex mutex_;
  std::unordered_map<Key, Value, Hash> map_;
};

/// 分片并发Map（更高并发度）
template <typename Key, typename Value, size_t ShardCount = 16>
class ShardedMap {
public:
  void insert(const Key &key, const Value &value) {
    getShard(key).insert(key, value);
  }

  std::optional<Value> get(const Key &key) const {
    return getShard(key).get(key);
  }

  bool erase(const Key &key) { return getShard(key).erase(key); }

  bool contains(const Key &key) const { return getShard(key).contains(key); }

  size_t size() const {
    size_t total = 0;
    for (const auto &shard : shards_)
      total += shard.size();
    return total;
  }

private:
  ConcurrentMap<Key, Value> &getShard(const Key &key) {
    return shards_[std::hash<Key>{}(key) % ShardCount];
  }

  const ConcurrentMap<Key, Value> &getShard(const Key &key) const {
    return shards_[std::hash<Key>{}(key) % ShardCount];
  }

  mutable std::array<ConcurrentMap<Key, Value>, ShardCount> shards_;
};

} // namespace container
} // namespace darwincore

#endif // DARWINCORE_CONCURRENT_MAP_H
