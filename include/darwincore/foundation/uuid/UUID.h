//
// UUID.h
// DarwinCore
//

#ifndef DARWINCORE_UUID_H
#define DARWINCORE_UUID_H

#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace darwincore {
namespace uuid {

/// UUID 类（符合 RFC 4122）
class UUID {
public:
  /**
   * @brief 生成随机 UUID（版本 4）
   */
  static UUID generate();

  /**
   * @brief 从字符串解析 UUID
   * @param str UUID 字符串（支持标准格式和紧凑格式）
   * @return 包含 UUID 的 optional，解析失败返回 std::nullopt
   */
  static std::optional<UUID> parse(const std::string &str);

  /**
   * @brief 获取 nil UUID（全零）
   */
  static UUID nil();

  /**
   * @brief 默认构造（产生 nil UUID）
   */
  UUID();

  /**
   * @brief 转换为标准格式字符串（8-4-4-4-12）
   */
  std::string toString() const;

  /**
   * @brief 转换为不带连字符的字符串
   */
  std::string toCompactString() const;

  /**
   * @brief 获取版本号
   */
  int version() const { return (data_[6] >> 4) & 0x0F; }

  /**
   * @brief 获取变体号
   */
  int variant() const;

  /**
   * @brief 检查是否为 nil UUID
   */
  bool isNil() const;

  /**
   * @brief 获取原始数据
   */
  const std::array<uint8_t, 16> &data() const { return data_; }

  /// 比较运算符
  bool operator==(const UUID &other) const { return data_ == other.data_; }
  bool operator!=(const UUID &other) const { return data_ != other.data_; }
  bool operator<(const UUID &other) const { return data_ < other.data_; }

  /**
   * @brief 计算哈希值
   */
  size_t hash() const;

private:
  std::array<uint8_t, 16> data_;
};

} // namespace uuid
} // namespace darwincore

// std::hash 特化
namespace std {
template <> struct hash<darwincore::uuid::UUID> {
  size_t operator()(const darwincore::uuid::UUID &uuid) const {
    return uuid.hash();
  }
};
} // namespace std

#endif // DARWINCORE_UUID_H
