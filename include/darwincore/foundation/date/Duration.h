//
// Duration.h
// DarwinCore
//

#ifndef DARWINCORE_DURATION_H
#define DARWINCORE_DURATION_H

#include <chrono>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>

namespace darwincore {
namespace date {

/// 时间间隔类
class Duration {
public:
  using Nanoseconds = std::chrono::nanoseconds;
  using Microseconds = std::chrono::microseconds;
  using Milliseconds = std::chrono::milliseconds;
  using Seconds = std::chrono::seconds;
  using Minutes = std::chrono::minutes;
  using Hours = std::chrono::hours;

  // ========================================
  // 工厂方法
  // ========================================

  static Duration nanoseconds(int64_t n) { return Duration(Nanoseconds(n)); }
  static Duration microseconds(int64_t us) {
    return Duration(Microseconds(us));
  }
  static Duration milliseconds(int64_t ms) {
    return Duration(Milliseconds(ms));
  }
  static Duration seconds(int64_t s) { return Duration(Seconds(s)); }
  static Duration minutes(int64_t m) { return Duration(Minutes(m)); }
  static Duration hours(int64_t h) { return Duration(Hours(h)); }
  static Duration days(int64_t d) { return Duration(Hours(d * 24)); }
  static Duration weeks(int64_t w) { return Duration(Hours(w * 24 * 7)); }

  static Duration zero() { return Duration(Nanoseconds(0)); }

  // ========================================
  // 构造
  // ========================================

  Duration() : nanos_(0) {}

  template <typename Rep, typename Period>
  explicit Duration(std::chrono::duration<Rep, Period> d)
      : nanos_(std::chrono::duration_cast<Nanoseconds>(d)) {}

  // ========================================
  // 转换
  // ========================================

  [[nodiscard]] int64_t toNanoseconds() const { return nanos_.count(); }
  [[nodiscard]] int64_t toMicroseconds() const {
    return std::chrono::duration_cast<Microseconds>(nanos_).count();
  }
  [[nodiscard]] int64_t toMilliseconds() const {
    return std::chrono::duration_cast<Milliseconds>(nanos_).count();
  }
  [[nodiscard]] int64_t toSeconds() const {
    return std::chrono::duration_cast<Seconds>(nanos_).count();
  }
  [[nodiscard]] int64_t toMinutes() const {
    return std::chrono::duration_cast<Minutes>(nanos_).count();
  }
  [[nodiscard]] int64_t toHours() const {
    return std::chrono::duration_cast<Hours>(nanos_).count();
  }
  [[nodiscard]] int64_t toDays() const { return toHours() / 24; }
  [[nodiscard]] int64_t toWeeks() const { return toDays() / 7; }

  [[nodiscard]] double toSecondsDouble() const {
    return std::chrono::duration<double>(nanos_).count();
  }

  // ========================================
  // 运算
  // ========================================

  Duration operator+(const Duration &other) const {
    return Duration(nanos_ + other.nanos_);
  }

  Duration operator-(const Duration &other) const {
    return Duration(nanos_ - other.nanos_);
  }

  Duration operator*(int64_t factor) const {
    return Duration(Nanoseconds(nanos_.count() * factor));
  }

  Duration operator/(int64_t divisor) const {
    return Duration(Nanoseconds(nanos_.count() / divisor));
  }

  Duration &operator+=(const Duration &other) {
    nanos_ += other.nanos_;
    return *this;
  }

  Duration &operator-=(const Duration &other) {
    nanos_ -= other.nanos_;
    return *this;
  }

  Duration abs() const {
    return Duration(Nanoseconds(std::abs(nanos_.count())));
  }

  // ========================================
  // 比较
  // ========================================

  bool operator==(const Duration &other) const {
    return nanos_ == other.nanos_;
  }
  bool operator!=(const Duration &other) const {
    return nanos_ != other.nanos_;
  }
  bool operator<(const Duration &other) const { return nanos_ < other.nanos_; }
  bool operator<=(const Duration &other) const {
    return nanos_ <= other.nanos_;
  }
  bool operator>(const Duration &other) const { return nanos_ > other.nanos_; }
  bool operator>=(const Duration &other) const {
    return nanos_ >= other.nanos_;
  }

  [[nodiscard]] bool isNegative() const { return nanos_.count() < 0; }
  [[nodiscard]] bool isZero() const { return nanos_.count() == 0; }
  [[nodiscard]] bool isPositive() const { return nanos_.count() > 0; }

  // ========================================
  // 格式化
  // ========================================

  /// 格式化为人类可读字符串
  [[nodiscard]] std::string toString() const;

  /// 格式化为 HH:MM:SS
  [[nodiscard]] std::string toHHMMSS() const;

  /// 格式化为 ISO 8601 持续时间
  [[nodiscard]] std::string toISO8601() const;

private:
  Nanoseconds nanos_;
};

} // namespace date
} // namespace darwincore

#endif // DARWINCORE_DURATION_H
