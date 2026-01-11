//
// BitSet.h
// DarwinCore
//

#ifndef DARWINCORE_BITSET_H
#define DARWINCORE_BITSET_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace darwincore {
namespace container {

/**
 * @brief 动态位集合 (Dynamic BitSet)
 *
 * 实用开发建议与使用场景：
 * 1. 权限控制列表
 * (ACL)：用每一个位代表一个特定操作（读、写、删等），极大地压缩权限存储空间。
 * 2.
 * 状态标志数组：在处理大量具有二状态特征的对象时（如：数万个并发任务的完成状态），BitSet
 * 比 boolean 数组节省 8 倍内存。
 * 3.
 * 布隆过滤器/位索引：作为搜索引擎或大型数据库的底层数据结构，用于快速判断元素是否存在。
 * 4. 资源调度：用于管理位图内存分配或任务调度器中的 CPU 闲忙状态检测。
 */
class BitSet {
public:
  explicit BitSet(size_t size = 0) : size_(size), bits_((size + 63) / 64, 0) {}

  void set(size_t pos, bool value = true) {
    checkBounds(pos);
    if (value)
      bits_[pos / 64] |= (1ULL << (pos % 64));
    else
      bits_[pos / 64] &= ~(1ULL << (pos % 64));
  }

  [[nodiscard]] bool get(size_t pos) const {
    checkBounds(pos);
    return (bits_[pos / 64] >> (pos % 64)) & 1;
  }

  [[nodiscard]] bool operator[](size_t pos) const { return get(pos); }

  void flip(size_t pos) {
    checkBounds(pos);
    bits_[pos / 64] ^= (1ULL << (pos % 64));
  }

  void flipAll() {
    for (auto &word : bits_)
      word = ~word;
    clearUnusedBits();
  }

  void setAll() {
    for (auto &word : bits_)
      word = ~0ULL;
    clearUnusedBits();
  }

  void reset() {
    for (auto &word : bits_)
      word = 0;
  }

  [[nodiscard]] size_t count() const {
    size_t total = 0;
    for (auto word : bits_)
      total += __builtin_popcountll(word);
    return total;
  }

  [[nodiscard]] bool any() const {
    for (auto word : bits_)
      if (word)
        return true;
    return false;
  }

  [[nodiscard]] bool none() const { return !any(); }

  [[nodiscard]] bool all() const { return count() == size_; }

  [[nodiscard]] size_t size() const noexcept { return size_; }

  void resize(size_t newSize) {
    bits_.resize((newSize + 63) / 64, 0);
    size_ = newSize;
    clearUnusedBits();
  }

  BitSet operator&(const BitSet &other) const;
  BitSet operator|(const BitSet &other) const;
  BitSet operator^(const BitSet &other) const;

  [[nodiscard]] std::string toString() const;

  /**
   * @brief 查找第一个设置的位
   * @return 位索引，如果没有设置位则返回 size()
   */
  [[nodiscard]] size_t findFirst() const;

private:
  void checkBounds(size_t pos) const {
    if (pos >= size_)
      throw std::out_of_range("BitSet index out of range");
  }

  void clearUnusedBits() {
    if (size_ % 64 != 0 && !bits_.empty())
      bits_.back() &= (1ULL << (size_ % 64)) - 1;
  }

  size_t size_;
  std::vector<uint64_t> bits_;
};

} // namespace container
} // namespace darwincore

#endif // DARWINCORE_BITSET_H
