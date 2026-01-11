//
// Calendar.cpp
// DarwinCore
//

#include <darwincore/foundation/date/Calendar.h>

namespace darwincore {
namespace date {

int Calendar::daysInMonth(int year, int month) {
  static const int days[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month < 1 || month > 12)
    return 0;
  if (month == 2 && isLeapYear(year))
    return 29;
  return days[month];
}

bool Calendar::isLeapYear(int year) {
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int Calendar::dayOfWeek(int year, int month, int day) {
  if (month < 3) {
    month += 12;
    year--;
  }
  int k = year % 100;
  int j = year / 100;
  int h = (day + (13 * (month + 1)) / 5 + k + k / 4 + j / 4 - 2 * j) % 7;
  return (h + 6) % 7;
}

std::string Calendar::dayOfWeekName(int dow, bool abbreviated) {
  static const char *full[] = {"Sunday",   "Monday", "Tuesday", "Wednesday",
                               "Thursday", "Friday", "Saturday"};
  static const char *abbr[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  if (dow < 0 || dow > 6)
    return "";
  return abbreviated ? abbr[dow] : full[dow];
}

std::string Calendar::monthName(int month, bool abbreviated) {
  static const char *full[] = {
      "",     "January", "February",  "March",   "April",    "May",     "June",
      "July", "August",  "September", "October", "November", "December"};
  static const char *abbr[] = {"",    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                               "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  if (month < 1 || month > 12)
    return "";
  return abbreviated ? abbr[month] : full[month];
}

int Calendar::weeksInMonth(int year, int month) {
  int firstDay = firstDayOfMonth(year, month);
  int days = daysInMonth(year, month);
  return (firstDay + days + 6) / 7;
}

void Calendar::addDays(int &year, int &month, int &day, int days) {
  int jd = toJulianDay(year, month, day) + days;
  fromJulianDay(jd, year, month, day);
}

void Calendar::addMonths(int &year, int &month, int &day, int months) {
  month += months;
  while (month > 12) {
    month -= 12;
    year++;
  }
  while (month < 1) {
    month += 12;
    year--;
  }
  int maxDay = daysInMonth(year, month);
  if (day > maxDay)
    day = maxDay;
}

int Calendar::weekOfYear(int year, int month, int day) {
  int doy = dayOfYear(year, month, day);
  int dow = dayOfWeek(year, month, day);
  int woy = (10 + doy - dow) / 7;
  if (woy == 0)
    return weekOfYear(year - 1, 12, 31);
  if (woy == 53) {
    int jan1dow = dayOfWeek(year + 1, 1, 1);
    if (jan1dow <= 4)
      return 1;
  }
  return woy;
}

int Calendar::dayOfYear(int year, int month, int day) {
  static const int daysBeforeMonth[] = {0,   0,   31,  59,  90,  120, 151,
                                        181, 212, 243, 273, 304, 334};
  int doy = daysBeforeMonth[month] + day;
  if (month > 2 && isLeapYear(year))
    doy++;
  return doy;
}

void Calendar::nextWeekday(int &year, int &month, int &day) {
  do {
    addDays(year, month, day, 1);
  } while (isWeekend(year, month, day));
}

int Calendar::toJulianDay(int year, int month, int day) {
  int a = (14 - month) / 12;
  int y = year + 4800 - a;
  int m = month + 12 * a - 3;
  return day + (153 * m + 2) / 5 + 365 * y + y / 4 - y / 100 + y / 400 - 32045;
}

void Calendar::fromJulianDay(int jd, int &year, int &month, int &day) {
  int a = jd + 32044;
  int b = (4 * a + 3) / 146097;
  int c = a - 146097 * b / 4;
  int d = (4 * c + 3) / 1461;
  int e = c - 1461 * d / 4;
  int m = (5 * e + 2) / 153;
  day = e - (153 * m + 2) / 5 + 1;
  month = m + 3 - 12 * (m / 10);
  year = 100 * b + d - 4800 + m / 10;
}

} // namespace date
} // namespace darwincore
