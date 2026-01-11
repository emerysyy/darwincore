//
// Hash.h
// DarwinCore
//

#ifndef DARWINCORE_HASH_H
#define DARWINCORE_HASH_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace darwincore {
namespace algorithm {

/// 非加密哈希算法集合
class Hash {
public:
  /**
   * @brief FNV-1a 哈希（32位）
   */
  static uint32_t fnv1a32(const void *data, size_t length);
  static uint32_t fnv1a32(std::string_view str) {
    return fnv1a32(str.data(), str.size());
  }

  /**
   * @brief FNV-1a 哈希（64位）
   */
  static uint64_t fnv1a64(const void *data, size_t length);
  static uint64_t fnv1a64(std::string_view str) {
    return fnv1a64(str.data(), str.size());
  }

  /**
   * @brief MurmurHash3（32位）
   */
  static uint32_t murmur3_32(const void *data, size_t length,
                             uint32_t seed = 0);
  static uint32_t murmur3_32(std::string_view str, uint32_t seed = 0) {
    return murmur3_32(str.data(), str.size(), seed);
  }

  /**
   * @brief DJB2 哈希
   */
  static uint32_t djb2(std::string_view str);

  /**
   * @brief CRC32（查表法）
   */
  static uint32_t crc32(const void *data, size_t length);

  /**
   * @brief 组合多个哈希值
   */
  static size_t combine(size_t seed, size_t hash) {
    return seed ^ (hash + 0x9e3779b9 + (seed << 6) + (seed >> 2));
  }

private:
  static uint32_t rotl32(uint32_t x, int8_t r);
  static uint32_t fmix32(uint32_t h);
};

} // namespace algorithm
} // namespace darwincore

#endif // DARWINCORE_HASH_H
