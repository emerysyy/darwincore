//
// TimeZone.h
// DarwinCore
//

#ifndef DARWINCORE_TIME_ZONE_H
#define DARWINCORE_TIME_ZONE_H

#include <chrono>
#include <ctime>
#include <optional>
#include <string>

namespace darwincore {
namespace date {

/// 时区类
class TimeZone {
public:
  /// 获取系统本地时区
  static TimeZone local();

  /// 获取 UTC 时区
  static TimeZone utc();

  /// 从偏移量创建（秒）
  static TimeZone fromOffset(int offsetSeconds);

  /// 从偏移量创建（小时）
  static TimeZone fromOffsetHours(int hours, int minutes = 0) {
    int totalSeconds = hours * 3600 + (hours >= 0 ? minutes : -minutes) * 60;
    return fromOffset(totalSeconds);
  }

  /// 从 IANA 时区名称创建（简化实现）
  static std::optional<TimeZone> fromName(const std::string &name);

  /// 默认构造（UTC）
  TimeZone() : offsetSeconds_(0), name_("UTC"), isDST_(false) {}

  /// 获取偏移量（秒）
  [[nodiscard]] int offsetSeconds() const { return offsetSeconds_; }

  /// 获取偏移量（小时）
  [[nodiscard]] double offsetHours() const { return offsetSeconds_ / 3600.0; }

  /// 获取时区名称
  [[nodiscard]] const std::string &name() const { return name_; }

  /// 是否为夏令时
  [[nodiscard]] bool isDST() const { return isDST_; }

  /// 格式化为 +HH:MM 格式
  [[nodiscard]] std::string format() const;

  /// 转换时间戳
  [[nodiscard]] std::time_t toUTC(std::time_t localTime) const {
    return localTime - offsetSeconds_;
  }

  [[nodiscard]] std::time_t fromUTC(std::time_t utcTime) const {
    return utcTime + offsetSeconds_;
  }

  bool operator==(const TimeZone &other) const {
    return offsetSeconds_ == other.offsetSeconds_;
  }

private:
  int offsetSeconds_;
  std::string name_;
  bool isDST_;
};

} // namespace date
} // namespace darwincore

#endif // DARWINCORE_TIME_ZONE_H
