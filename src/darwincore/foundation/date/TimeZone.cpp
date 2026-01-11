//
// TimeZone.cpp
// DarwinCore
//

#include <darwincore/foundation/date/TimeZone.h>
#include <cmath>
#include <cstdio>

namespace darwincore {
namespace date {

TimeZone TimeZone::local() {
  TimeZone tz;
  std::time_t now = std::time(nullptr);
  std::tm local_tm, utc_tm;
  localtime_r(&now, &local_tm);
  gmtime_r(&now, &utc_tm);

  tz.offsetSeconds_ = static_cast<int>(
      std::difftime(std::mktime(&local_tm), std::mktime(&utc_tm)));
  tz.name_ = local_tm.tm_zone ? local_tm.tm_zone : "Local";
  tz.isDST_ = local_tm.tm_isdst > 0;
  return tz;
}

TimeZone TimeZone::utc() {
  TimeZone tz;
  tz.offsetSeconds_ = 0;
  tz.name_ = "UTC";
  tz.isDST_ = false;
  return tz;
}

TimeZone TimeZone::fromOffset(int offsetSeconds) {
  TimeZone tz;
  tz.offsetSeconds_ = offsetSeconds;

  int hours = std::abs(offsetSeconds) / 3600;
  int minutes = (std::abs(offsetSeconds) % 3600) / 60;
  char buf[16];
  std::snprintf(buf, sizeof(buf), "%s%02d:%02d", offsetSeconds >= 0 ? "+" : "-",
                hours, minutes);
  tz.name_ = buf;
  return tz;
}

std::optional<TimeZone> TimeZone::fromName(const std::string &name) {
  if (name == "UTC" || name == "GMT")
    return utc();
  if (name == "Asia/Shanghai" || name == "CST")
    return fromOffsetHours(8);
  if (name == "America/New_York" || name == "EST")
    return fromOffsetHours(-5);
  if (name == "America/Los_Angeles" || name == "PST")
    return fromOffsetHours(-8);
  if (name == "Europe/London")
    return fromOffsetHours(0);
  if (name == "Europe/Paris" || name == "CET")
    return fromOffsetHours(1);
  if (name == "Asia/Tokyo" || name == "JST")
    return fromOffsetHours(9);
  return std::nullopt;
}

std::string TimeZone::format() const {
  int hours = std::abs(offsetSeconds_) / 3600;
  int minutes = (std::abs(offsetSeconds_) % 3600) / 60;
  char buf[16];
  std::snprintf(buf, sizeof(buf), "%s%02d:%02d",
                offsetSeconds_ >= 0 ? "+" : "-", hours, minutes);
  return buf;
}

} // namespace date
} // namespace darwincore
