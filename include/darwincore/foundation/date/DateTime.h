//
// DateTime.h
// DarwinCore
//
// 完全重构的日期时间类

#ifndef DARWINCORE_DATE_TIME_H
#define DARWINCORE_DATE_TIME_H

#include <chrono>
#include <ctime>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>

namespace darwincore {
namespace date {

// 前向声明
class Duration;
class TimeZone;

  /**
   * @brief 日期时间类（不可变、基于绝对时间点）
   *
   * 设计语义说明：
   * ------------------------------------------------------------
   * DateTime 内部基于 std::chrono::system_clock::time_point，
   * 表示一个“绝对时间点（Instant）”，等价于 Unix 时间戳语义：
   *
   *   - 自 1970-01-01 00:00:00 UTC 起的持续时间
   *   - 内部存储与时区无关
   *   - 时间戳在全球范围内唯一且一致
   *
   * 时区语义：
   * ------------------------------------------------------------
   * - DateTime 本身【不持有时区信息】
   * - 所有年月日、时分秒拆解、格式化操作
   *   均基于【当前进程的本地系统时区】
   * - 夏令时（DST）行为由系统 C 库负责
   *
   * 适用场景：
   * ------------------------------------------------------------
   * - 日志时间戳
   * - 跨进程 / 跨主机时间传输
   * - 排序、比较、时间差计算
   *
   * 不适用场景：
   * ------------------------------------------------------------
   * - 明确的时区计算（如 Asia/Shanghai → Europe/London）
   * - 历史时区规则精确回溯
   * - 多时区并行展示
   *
   * 若需要显式时区支持，应在更高层引入 ZonedDateTime / TimeZone。
   */
class DateTime {
public:
  using Clock = std::chrono::system_clock;
  using TimePoint = Clock::time_point;
  using Nanoseconds = std::chrono::nanoseconds;

  /**
 * @brief 获取当前系统时间
 *
 * @return 当前时间点（绝对时间，等价于 UTC Instant）
 *
 * 注意：
 * - 返回值不包含时区信息
 * - 展示为本地时间时，使用系统当前时区
 */
  static DateTime now();

  /// 从时间戳创建（秒）
  static DateTime fromTimestamp(int64_t seconds) {
    return DateTime(TimePoint(std::chrono::seconds(seconds)));
  }

  /// 从时间戳创建（毫秒）
  static DateTime fromTimestampMs(int64_t milliseconds) {
    return DateTime(TimePoint(std::chrono::milliseconds(milliseconds)));
  }

  /// 从组件创建
  static DateTime fromComponents(int year, int month, int day, int hour = 0,
                                 int minute = 0, int second = 0);

  /// 从 ISO 8601 字符串解析
  static std::optional<DateTime> parse(const std::string &str,
                        const std::string &fmt = "%Y-%m-%d %H:%M:%S");

  /// 检查 DateTime 是否有效
  bool isValid() const { return timePoint_ != TimePoint{}; }

  /// 默认构造（epoch）
  DateTime() : timePoint_(TimePoint{}) {}

  /// 从 TimePoint 构造
  explicit DateTime(TimePoint tp) : timePoint_(tp) {}

  // ========================================
  // 获取组件
  // ========================================

  int year() const;
  int month() const;     // 1-12
  int day() const;       // 1-31
  int hour() const;      // 0-23
  int minute() const;    // 0-59
  int second() const;    // 0-59
  int dayOfWeek() const; // 0=周日
  int dayOfYear() const; // 1-366

  /**
  * @brief 当前时间是否处于夏令时（DST）
  *
  * @return true 表示系统本地时区当前启用了 DST
  */
  bool isDST() const;

  int millisecond() const;

  // ========================================
  // 时间戳
  // ========================================

  int64_t timestamp() const;
  int64_t timestampMs() const;
  int64_t timestampUs() const;

  [[nodiscard]] TimePoint timePoint() const { return timePoint_; }

  // ========================================
  // 格式化
  // ========================================

/**
 * @brief 按指定格式将时间格式化为字符串（基于本地时区）
 *
 * 格式化规则：
 * ------------------------------------------------------------
 * 使用标准 C / POSIX `strftime` 语义，所有字段均基于
 * 当前系统本地时区计算。
 *
 * ⚠ 注意：
 * - DateTime 本身不持有时区信息
 * - %Z / %z 等时区相关字段来自系统本地时区
 *
 * ========================
 * 年（Year）
 * ========================
 * %Y  四位年份                示例：2026
 * %y  两位年份                示例：26
 * %C  世纪（year / 100）      示例：20
 *
 * ========================
 * 月（Month）
 * ========================
 * %m  月份（01–12）           示例：01
 * %B  月份全名                示例：January
 * %b  月份缩写                示例：Jan
 * %h  月份缩写（等同 %b）
 *
 * ========================
 * 日（Day）
 * ========================
 * %d  当月第几天（01–31）     示例：08
 * %e  当月第几天（空格填充）  示例： 8
 * %j  当年第几天（001–366）   示例：008
 *
 * ========================
 * 星期（Weekday）
 * ========================
 * %A  星期全名                示例：Thursday
 * %a  星期缩写                示例：Thu
 * %w  星期（0=周日）          示例：4
 * %u  星期（1=周一，ISO）     示例：4
 *
 * ========================
 * 周（Week Number）
 * ========================
 * %U  周数（周日为一周起始）
 * %W  周数（周一为一周起始）
 * %V  ISO 8601 周数（01–53）
 * %G  ISO 周年份（四位）
 * %g  ISO 周年份（两位）
 *
 * ========================
 * 时间（Hour / Minute / Second）
 * ========================
 * %H  24 小时制小时（00–23）  示例：14
 * %I  12 小时制小时（01–12）  示例：02
 * %M  分钟（00–59）           示例：05
 * %S  秒（00–60，含闰秒）     示例：09
 * %p  AM / PM                示例：PM
 *
 * ========================
 * 时区（来自系统本地时区）
 * ========================
 * %Z  时区名称                示例：CST / GMT / JST
 * %z  UTC 偏移                示例：+0800
 *
 * ========================
 * 组合 / 快捷格式
 * ========================
 * %F  等同 %Y-%m-%d
 * %T  等同 %H:%M:%S
 * %R  等同 %H:%M
 * %D  等同 %m/%d/%y
 * %c  本地完整日期时间表示
 * %x  本地日期表示
 * %X  本地时间表示
 *
 * ========================
 * 字面量与特殊字符
 * ========================
 * %%  字符 '%'
 * \\n 换行
 * \\t 制表符
 *
 * 示例：
 * ------------------------------------------------------------
 * format("%Y-%m-%d %H:%M:%S") → 2026-01-08 12:34:56
 * format("%FT%T%z")           → 2026-01-08T12:34:56+0800
 */
  std::string format(const std::string &fmt = "%Y-%m-%d %H:%M:%S") const;

  std::string toISOString() const { return format("%Y-%m-%dT%H:%M:%S"); }

  std::string toDateString() const { return format("%Y-%m-%d"); }

  std::string toTimeString() const { return format("%H:%M:%S"); }

  // ========================================
  // 运算
  // ========================================

  DateTime addYears(int years) const {
    return fromComponents(year() + years, month(), day(), hour(), minute(),
                          second());
  }

  DateTime addMonths(int months) const;

  DateTime addDays(int days) const {
    return DateTime(timePoint_ + std::chrono::hours(24 * days));
  }

  DateTime addHours(int hours) const {
    return DateTime(timePoint_ + std::chrono::hours(hours));
  }

  DateTime addMinutes(int minutes) const {
    return DateTime(timePoint_ + std::chrono::minutes(minutes));
  }

  DateTime addSeconds(int seconds) const {
    return DateTime(timePoint_ + std::chrono::seconds(seconds));
  }

  DateTime addMilliseconds(int64_t ms) const {
    return DateTime(timePoint_ + std::chrono::milliseconds(ms));
  }

  /// 计算差值（秒）
  double diffSeconds(const DateTime &other) const {
    auto diff = timePoint_ - other.timePoint_;
    return std::chrono::duration<double>(diff).count();
  }

  /// 计算差值（天）
  double diffDays(const DateTime &other) const {
    return diffSeconds(other) / 86400.0;
  }

  // ========================================
  // 比较
  // ========================================

  bool operator==(const DateTime &other) const {
    return timePoint_ == other.timePoint_;
  }
  bool operator!=(const DateTime &other) const { return !(*this == other); }
  bool operator<(const DateTime &other) const {
    return timePoint_ < other.timePoint_;
  }
  bool operator<=(const DateTime &other) const {
    return timePoint_ <= other.timePoint_;
  }
  bool operator>(const DateTime &other) const {
    return timePoint_ > other.timePoint_;
  }
  bool operator>=(const DateTime &other) const {
    return timePoint_ >= other.timePoint_;
  }

  // ========================================
  // 判断
  // ========================================

  bool isLeapYear() const;

  bool isWeekend() const {
    int dow = dayOfWeek();
    return dow == 0 || dow == 6;
  }

  bool isToday() const;

  /// 获取当月第一天
  DateTime startOfMonth() const { return fromComponents(year(), month(), 1); }

  /// 获取当月最后一天
  DateTime endOfMonth() const;

  /// 获取当天开始
  DateTime startOfDay() const { return fromComponents(year(), month(), day()); }

  /// 获取当天结束
  DateTime endOfDay() const {
    return fromComponents(year(), month(), day(), 23, 59, 59);
  }

private:
  std::tm localTm() const;

  TimePoint timePoint_;
};

} // namespace date
} // namespace darwincore

#endif // DARWINCORE_DATE_TIME_H
