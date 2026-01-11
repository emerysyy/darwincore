//
// LRUCache.h
// DarwinCore
//
// Created by DarwinCore on 2026/1/7.
// Copyright © 2026 DarwinCore. All rights reserved.
//

#ifndef DARWINCORE_LRU_CACHE_H
#define DARWINCORE_LRU_CACHE_H

#include <cstddef>
#include <functional>
#include <list>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <unordered_map>

namespace darwincore {
namespace container {

/**
 * @brief LRU（Least Recently Used）缓存
 *
 * 基于双向链表和哈希表实现的 LRU 缓存。
 * 当缓存满时，自动淘汰最久未使用的元素。
 *
 * @tparam Key 键类型（需支持 hash 和相等比较）
 * @tparam Value 值类型
 *
 * 特性：
 * - O(1) 时间复杂度的查找、插入、删除
 * - 可配置的淘汰回调
 * - 线程不安全，需要外部同步（或使用 ThreadSafeLRUCache）
 *
 * 使用示例：
 * @code
 *     LRUCache<std::string, int> cache(100);
 *     cache.put("key", 42);
 *     auto value = cache.get("key");
 * @endcode
 */
template <typename Key, typename Value> class LRUCache {
public:
  using EvictionCallback = std::function<void(const Key &, Value &)>;

  /**
   * @brief 构造函数
   * @param capacity 缓存容量（必须大于0）
   * @throws std::invalid_argument 如果容量为0
   */
  explicit LRUCache(size_t capacity) : capacity_(capacity) {
    if (capacity == 0) {
      throw std::invalid_argument("LRUCache capacity must be greater than 0");
    }
  }

  ~LRUCache() = default;

  // 禁用拷贝
  LRUCache(const LRUCache &) = delete;
  LRUCache &operator=(const LRUCache &) = delete;

  // 支持移动
  LRUCache(LRUCache &&other) noexcept
      : capacity_(other.capacity_), list_(std::move(other.list_)),
        map_(std::move(other.map_)),
        evictionCallback_(std::move(other.evictionCallback_)) {
    other.capacity_ = 0;
  }

  LRUCache &operator=(LRUCache &&other) noexcept {
    if (this != &other) {
      capacity_ = other.capacity_;
      list_ = std::move(other.list_);
      map_ = std::move(other.map_);
      evictionCallback_ = std::move(other.evictionCallback_);
      other.capacity_ = 0;
    }
    return *this;
  }

  /**
   * @brief 设置淘汰回调函数
   * @param callback 当元素被淘汰时调用的函数
   */
  void setEvictionCallback(EvictionCallback callback) {
    evictionCallback_ = std::move(callback);
  }

  /**
   * @brief 获取缓存中的值
   *
   * 如果键存在，将该元素移动到最近使用的位置。
   *
   * @param key 键
   * @return 包含值的 optional，如果键不存在则返回 std::nullopt
   */
  std::optional<Value> get(const Key &key) {
    auto it = map_.find(key);
    if (it == map_.end()) {
      return std::nullopt;
    }

    // 移动到链表头部（最近使用）
    list_.splice(list_.begin(), list_, it->second);
    return it->second->second;
  }

  /**
   * @brief 获取值的引用（不检查是否存在）
   *
   * @param key 键
   * @return 值的引用
   * @throws std::out_of_range 如果键不存在
   */
  Value &at(const Key &key) {
    auto it = map_.find(key);
    if (it == map_.end()) {
      throw std::out_of_range("Key not found in LRUCache");
    }
    list_.splice(list_.begin(), list_, it->second);
    return it->second->second;
  }

  const Value &at(const Key &key) const {
    auto it = map_.find(key);
    if (it == map_.end()) {
      throw std::out_of_range("Key not found in LRUCache");
    }
    return it->second->second;
  }

  /**
   * @brief 插入或更新缓存
   *
   * 如果键已存在，更新值并移动到最近使用位置。
   * 如果键不存在且缓存已满，淘汰最久未使用的元素。
   *
   * @param key 键
   * @param value 值
   */
  void put(const Key &key, const Value &value) {
    auto it = map_.find(key);

    if (it != map_.end()) {
      // 键已存在，更新值并移动到头部
      it->second->second = value;
      list_.splice(list_.begin(), list_, it->second);
      return;
    }

    // 如果缓存已满，淘汰最久未使用的元素
    if (map_.size() >= capacity_) {
      evict();
    }

    // 插入新元素到头部
    list_.emplace_front(key, value);
    map_[key] = list_.begin();
  }

  /**
   * @brief 插入或更新缓存（移动语义）
   */
  void put(const Key &key, Value &&value) {
    auto it = map_.find(key);

    if (it != map_.end()) {
      it->second->second = std::move(value);
      list_.splice(list_.begin(), list_, it->second);
      return;
    }

    if (map_.size() >= capacity_) {
      evict();
    }

    list_.emplace_front(key, std::move(value));
    map_[key] = list_.begin();
  }

  /**
   * @brief 检查键是否存在（不影响 LRU 顺序）
   */
  [[nodiscard]] bool contains(const Key &key) const {
    return map_.find(key) != map_.end();
  }

  /**
   * @brief 删除指定键
   * @return true 如果键存在并被删除
   */
  bool erase(const Key &key) {
    auto it = map_.find(key);
    if (it == map_.end()) {
      return false;
    }

    list_.erase(it->second);
    map_.erase(it);
    return true;
  }

  /**
   * @brief 清空缓存
   */
  void clear() {
    list_.clear();
    map_.clear();
  }

  /**
   * @brief 获取当前缓存大小
   */
  [[nodiscard]] size_t size() const noexcept { return map_.size(); }

  /**
   * @brief 获取缓存容量
   */
  [[nodiscard]] size_t capacity() const noexcept { return capacity_; }

  /**
   * @brief 检查缓存是否为空
   */
  [[nodiscard]] bool empty() const noexcept { return map_.empty(); }

  /**
   * @brief 调整缓存容量
   *
   * 如果新容量小于当前大小，将淘汰多余元素。
   *
   * @param newCapacity 新容量
   */
  void resize(size_t newCapacity) {
    if (newCapacity == 0) {
      throw std::invalid_argument("LRUCache capacity must be greater than 0");
    }

    while (map_.size() > newCapacity) {
      evict();
    }
    capacity_ = newCapacity;
  }

  /**
   * @brief 获取或插入值
   *
   * 如果键存在，返回对应值的引用。
   * 如果键不存在，使用 factory 函数创建值并插入。
   *
   * @param key 键
   * @param factory 创建值的工厂函数
   * @return 值的引用
   */
  template <typename Factory>
  Value &getOrInsert(const Key &key, Factory &&factory) {
    auto it = map_.find(key);
    if (it != map_.end()) {
      list_.splice(list_.begin(), list_, it->second);
      return it->second->second;
    }

    if (map_.size() >= capacity_) {
      evict();
    }

    list_.emplace_front(key, factory());
    map_[key] = list_.begin();
    return list_.front().second;
  }

private:
  using ListType = std::list<std::pair<Key, Value>>;
  using MapType = std::unordered_map<Key, typename ListType::iterator>;

  /**
   * @brief 淘汰最久未使用的元素
   */
  void evict() {
    if (list_.empty()) {
      return;
    }

    auto &back = list_.back();
    if (evictionCallback_) {
      evictionCallback_(back.first, back.second);
    }
    map_.erase(back.first);
    list_.pop_back();
  }

  size_t capacity_;                   // 缓存容量
  ListType list_;                     // 双向链表（头部为最近使用）
  MapType map_;                       // 键到链表迭代器的映射
  EvictionCallback evictionCallback_; // 淘汰回调
};

/**
 * @brief 线程安全的 LRU 缓存
 *
 * 使用互斥锁保护的 LRU 缓存。
 */
template <typename Key, typename Value> class ThreadSafeLRUCache {
public:
  explicit ThreadSafeLRUCache(size_t capacity) : cache_(capacity) {}

  std::optional<Value> get(const Key &key) {
    std::lock_guard<std::mutex> lock(mutex_);
    return cache_.get(key);
  }

  void put(const Key &key, const Value &value) {
    std::lock_guard<std::mutex> lock(mutex_);
    cache_.put(key, value);
  }

  void put(const Key &key, Value &&value) {
    std::lock_guard<std::mutex> lock(mutex_);
    cache_.put(key, std::move(value));
  }

  bool contains(const Key &key) {
    std::lock_guard<std::mutex> lock(mutex_);
    return cache_.contains(key);
  }

  bool erase(const Key &key) {
    std::lock_guard<std::mutex> lock(mutex_);
    return cache_.erase(key);
  }

  void clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    cache_.clear();
  }

  size_t size() {
    std::lock_guard<std::mutex> lock(mutex_);
    return cache_.size();
  }

private:
  LRUCache<Key, Value> cache_;
  mutable std::mutex mutex_;
};

} // namespace container
} // namespace darwincore

#endif // DARWINCORE_LRU_CACHE_H
