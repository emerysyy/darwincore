//
// DateTime.cpp
// DarwinCore
//

#include <darwincore/foundation/date/DateTime.h>
#include <darwincore/foundation/date/Duration.h>
#include <darwincore/foundation/date/TimeZone.h>

namespace darwincore {
namespace date {

DateTime DateTime::now() {
  return DateTime(Clock::now());
}

DateTime DateTime::fromComponents(int year, int month, int day, int hour,
                                  int minute, int second) {
  std::tm tm = {};
  tm.tm_year = year - 1900;
  tm.tm_mon = month - 1;
  tm.tm_mday = day;
  tm.tm_hour = hour;
  tm.tm_min = minute;
  tm.tm_sec = second;
  std::time_t t = std::mktime(&tm);
  return fromTimestamp(t);
}

std::optional<DateTime> DateTime::parse(const std::string &str,
                                        const std::string &fmt) {
  std::tm tm = {};
  std::istringstream ss(str);
  ss >> std::get_time(&tm, fmt.c_str());
  if (ss.fail())
    return std::nullopt;
  return fromTimestamp(std::mktime(&tm));
}

int DateTime::year() const { return localTm().tm_year + 1900; }
int DateTime::month() const { return localTm().tm_mon + 1; }
int DateTime::day() const { return localTm().tm_mday; }
int DateTime::hour() const { return localTm().tm_hour; }
int DateTime::minute() const { return localTm().tm_min; }
int DateTime::second() const { return localTm().tm_sec; }
int DateTime::dayOfWeek() const { return localTm().tm_wday; }
int DateTime::dayOfYear() const { return localTm().tm_yday + 1; }
bool DateTime::isDST() const { return localTm().tm_isdst > 0; }

int DateTime::millisecond() const {
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                timePoint_.time_since_epoch()) %
            1000;
  return static_cast<int>(ms.count());
}

int64_t DateTime::timestamp() const {
  return std::chrono::duration_cast<std::chrono::seconds>(
             timePoint_.time_since_epoch())
      .count();
}

int64_t DateTime::timestampMs() const {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             timePoint_.time_since_epoch())
      .count();
}

int64_t DateTime::timestampUs() const {
  return std::chrono::duration_cast<std::chrono::microseconds>(
             timePoint_.time_since_epoch())
      .count();
}

std::string DateTime::format(const std::string &fmt) const {
  std::ostringstream oss;
  std::tm tm = localTm();
  oss << std::put_time(&tm, fmt.c_str());

  std::string result = oss.str();
  size_t msPos = result.find("%f");
  if (msPos != std::string::npos) {
    std::ostringstream msOss;
    msOss << std::setfill('0') << std::setw(3) << millisecond();
    result.replace(msPos, 2, msOss.str());
  }
  return result;
}

DateTime DateTime::addMonths(int months) const {
  int newMonth = month() + months;
  int newYear = year();
  while (newMonth > 12) {
    newMonth -= 12;
    ++newYear;
  }
  while (newMonth < 1) {
    newMonth += 12;
    --newYear;
  }
  return fromComponents(newYear, newMonth, day(), hour(), minute(), second());
}

bool DateTime::isLeapYear() const {
  int y = year();
  return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

DateTime DateTime::endOfMonth() const {
  static const int daysInMonth[] = {0,  31, 28, 31, 30, 31, 30,
                                    31, 31, 30, 31, 30, 31};
  int days = daysInMonth[month()];
  if (month() == 2 && isLeapYear())
    days = 29;
  return fromComponents(year(), month(), days, 23, 59, 59);
}

std::tm DateTime::localTm() const {
  std::time_t t = Clock::to_time_t(timePoint_);
  std::tm tm;
  localtime_r(&t, &tm);
  return tm;
}

} // namespace date
} // namespace darwincore
