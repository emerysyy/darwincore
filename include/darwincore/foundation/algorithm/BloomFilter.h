//
// BloomFilter.h
// DarwinCore
//

#ifndef DARWINCORE_BLOOM_FILTER_H
#define DARWINCORE_BLOOM_FILTER_H

#include <darwincore/foundation/algorithm/Hash.h>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace darwincore {
namespace algorithm {

  /**
   * @brief BloomFilter 是一种高性能、低内存占用的概率性集合成员判断结构。
   *
   * BloomFilter 用于判断某个元素“是否可能存在于一个集合中”，
   * 其核心特性如下：
   *
   * 1. 对“不存在”的判断是 100% 准确的（无 false negative）；
   * 2. 对“存在”的判断是概率性的，可能产生误报（false positive）；
   * 3. 不支持元素删除（非 Counting BloomFilter）；
   * 4. 通过位数组与多个哈希函数实现，查询与插入均为 O(k) 位操作；
   * 5. 适用于高并发、高频查询、集合规模较大的场景。
   *
   * 在安全与系统工程中，BloomFilter 通常不作为最终决策依据，
   * 而是作为“前置否定过滤器（Negative Pre-filter）”，
   * 用于快速排除“绝对不可能命中规则”的对象，从而显著降低
   * 后续精确匹配、规则引擎或策略判断的性能压力。
   *
   * BloomFilter 仅保证：
   *   - mightContain(x) == false → x 一定不在集合中
   * 不保证：
   *   - mightContain(x) == true  → x 一定在集合中
   *
   * 因此，BloomFilter 的典型使用模式为：
   *   - false → 直接放行或忽略
   *   - true  → 进入慢路径进行精确判断
   */
class BloomFilter {
public:
  /// 按预期元素数量和误报率构造
  BloomFilter(size_t expectedElements, double falsePositiveRate = 0.01);

  /// 按位数和哈希函数数量构造
  BloomFilter(size_t bitCount, size_t hashCount);

  /// 添加元素
  void add(const void *data, size_t length);

  void add(std::string_view str) { add(str.data(), str.size()); }

  /// false 一定正确，true 可能不正确
  /// 检查元素是否可能存在
  [[nodiscard]] bool mightContain(const void *data, size_t length) const;

  [[nodiscard]] bool mightContain(std::string_view str) const {
    return mightContain(str.data(), str.size());
  }

  /// 获取填充率
  [[nodiscard]] double fillRatio() const;

  /// 估算当前误报率
  [[nodiscard]] double estimatedFalsePositiveRate() const;

  /// 获取已添加元素数量
  [[nodiscard]] size_t count() const noexcept { return count_; }

  /// 获取位数组大小
  [[nodiscard]] size_t bitCount() const noexcept { return bits_.size(); }

  /// 获取哈希函数数量
  [[nodiscard]] size_t hashCount() const noexcept { return numHashes_; }

  /// 清空过滤器
  void clear();

  /// 合并两个布隆过滤器（两者必须具有相同参数）
  BloomFilter &merge(const BloomFilter &other);

private:
  /// 计算最优位数
  static size_t optimalBitCount(size_t n, double p);

  /// 计算最优哈希函数数量
  static size_t optimalHashCount(size_t n, double p);

  /// 双重哈希
  std::pair<uint64_t, uint64_t> doubleHash(const void *data,
                                           size_t length) const;

  size_t numHashes_;
  std::vector<bool> bits_;
  size_t count_ = 0;
};

} // namespace algorithm
} // namespace darwincore

#endif // DARWINCORE_BLOOM_FILTER_H
