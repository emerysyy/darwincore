//
// Calendar.h
// DarwinCore
//

#ifndef DARWINCORE_CALENDAR_H
#define DARWINCORE_CALENDAR_H

#include <ctime>
#include <string>
#include <vector>

namespace darwincore {
namespace date {

/// 日历计算工具
class Calendar {
public:
  /// 获取某月的天数
  static int daysInMonth(int year, int month);

  /// 获取某年的天数
  static int daysInYear(int year) { return isLeapYear(year) ? 366 : 365; }

  /// 判断是否为闰年
  static bool isLeapYear(int year);

  /// 获取星期几（0=周日，1=周一，...，6=周六）
  static int dayOfWeek(int year, int month, int day);

  /// 获取星期几名称
  static std::string dayOfWeekName(int dow, bool abbreviated = false);

  /// 获取月份名称
  static std::string monthName(int month, bool abbreviated = false);

  /// 获取某月第一天是星期几
  static int firstDayOfMonth(int year, int month) {
    return dayOfWeek(year, month, 1);
  }

  /// 获取某月的周数
  static int weeksInMonth(int year, int month);

  /// 计算两个日期之间的天数差
  static int daysBetween(int y1, int m1, int d1, int y2, int m2, int d2) {
    return toJulianDay(y2, m2, d2) - toJulianDay(y1, m1, d1);
  }

  /// 添加天数
  static void addDays(int &year, int &month, int &day, int days);

  /// 添加月数
  static void addMonths(int &year, int &month, int &day, int months);

  /// 获取一年中的第几周（ISO 8601）
  static int weekOfYear(int year, int month, int day);

  /// 获取一年中的第几天
  static int dayOfYear(int year, int month, int day);

  /// 判断是否为工作日（周一至周五）
  static bool isWeekday(int year, int month, int day) {
    int dow = dayOfWeek(year, month, day);
    return dow >= 1 && dow <= 5;
  }

  /// 判断是否为周末
  static bool isWeekend(int year, int month, int day) {
    return !isWeekday(year, month, day);
  }

  /// 获取下一个工作日
  static void nextWeekday(int &year, int &month, int &day);

private:
  /// 转换为儒略日数
  static int toJulianDay(int year, int month, int day);

  /// 从儒略日数转换
  static void fromJulianDay(int jd, int &year, int &month, int &day);
};

} // namespace date
} // namespace darwincore

#endif // DARWINCORE_CALENDAR_H
