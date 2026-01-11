//
// Duration.cpp
// DarwinCore
//

#include <darwincore/foundation/date/Duration.h>

namespace darwincore {
namespace date {

std::string Duration::toString() const {
  int64_t secs = toSeconds();
  if (std::abs(secs) < 60) {
    return std::to_string(toSecondsDouble()) + "s";
  }

  std::ostringstream oss;
  int64_t days = std::abs(secs) / 86400;
  int64_t hours = (std::abs(secs) % 86400) / 3600;
  int64_t minutes = (std::abs(secs) % 3600) / 60;
  int64_t seconds = std::abs(secs) % 60;

  if (isNegative())
    oss << "-";
  if (days > 0)
    oss << days << "d ";
  if (hours > 0 || days > 0)
    oss << hours << "h ";
  if (minutes > 0 || hours > 0 || days > 0)
    oss << minutes << "m ";
  oss << seconds << "s";

  return oss.str();
}

std::string Duration::toHHMMSS() const {
  int64_t secs = std::abs(toSeconds());
  int h = static_cast<int>(secs / 3600);
  int m = static_cast<int>((secs % 3600) / 60);
  int s = static_cast<int>(secs % 60);

  std::ostringstream oss;
  if (isNegative())
    oss << "-";
  oss << std::setfill('0') << std::setw(2) << h << ":" << std::setw(2) << m
      << ":" << std::setw(2) << s;
  return oss.str();
}

std::string Duration::toISO8601() const {
  std::ostringstream oss;
  oss << "P";
  int64_t secs = std::abs(toSeconds());
  int d = static_cast<int>(secs / 86400);
  secs %= 86400;
  if (d > 0)
    oss << d << "D";
  if (secs > 0 || d == 0) {
    oss << "T";
    int h = static_cast<int>(secs / 3600);
    secs %= 3600;
    int m = static_cast<int>(secs / 60);
    secs %= 60;
    if (h > 0)
      oss << h << "H";
    if (m > 0)
      oss << m << "M";
    oss << secs << "S";
  }
  return oss.str();
}

} // namespace date
} // namespace darwincore
