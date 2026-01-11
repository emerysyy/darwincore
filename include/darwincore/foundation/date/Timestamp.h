//
// Timestamp.h
// DarwinCore
//

#ifndef DARWINCORE_TIMESTAMP_H
#define DARWINCORE_TIMESTAMP_H

#include <chrono>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <sstream>
#include <string>

#ifdef __APPLE__
#include <mach/mach_time.h>
#endif

namespace darwincore {
namespace date {

/// 高精度时间戳
class Timestamp {
public:
  using Clock = std::chrono::high_resolution_clock;
  using TimePoint = Clock::time_point;
  using Nanoseconds = std::chrono::nanoseconds;

  /// 获取当前时间戳
  static Timestamp now() { return Timestamp(Clock::now()); }

  /// 从纳秒创建
  static Timestamp fromNanoseconds(int64_t ns) {
    return Timestamp(TimePoint(Nanoseconds(ns)));
  }

  /// 从微秒创建
  static Timestamp fromMicroseconds(int64_t us) {
    return Timestamp(TimePoint(std::chrono::microseconds(us)));
  }

  /// 从毫秒创建
  static Timestamp fromMilliseconds(int64_t ms) {
    return Timestamp(TimePoint(std::chrono::milliseconds(ms)));
  }

  /// 默认构造
  Timestamp() : timePoint_(TimePoint{}) {}

  /// 从 TimePoint 构造
  explicit Timestamp(TimePoint tp) : timePoint_(tp) {}

  // ========================================
  // 转换
  // ========================================

  [[nodiscard]] int64_t toNanoseconds() const {
    return std::chrono::duration_cast<Nanoseconds>(
               timePoint_.time_since_epoch())
        .count();
  }

  [[nodiscard]] int64_t toMicroseconds() const {
    return std::chrono::duration_cast<std::chrono::microseconds>(
               timePoint_.time_since_epoch())
        .count();
  }

  [[nodiscard]] int64_t toMilliseconds() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               timePoint_.time_since_epoch())
        .count();
  }

  [[nodiscard]] int64_t toSeconds() const {
    return std::chrono::duration_cast<std::chrono::seconds>(
               timePoint_.time_since_epoch())
        .count();
  }

  [[nodiscard]] double toSecondsDouble() const {
    return std::chrono::duration<double>(timePoint_.time_since_epoch()).count();
  }

  [[nodiscard]] TimePoint timePoint() const { return timePoint_; }

  // ========================================
  // 运算
  // ========================================

  /// 计算时间差（纳秒）
  [[nodiscard]] int64_t diffNanoseconds(const Timestamp &other) const;

  /// 计算时间差（微秒）
  [[nodiscard]] int64_t diffMicroseconds(const Timestamp &other) const;

  /// 计算时间差（毫秒）
  [[nodiscard]] int64_t diffMilliseconds(const Timestamp &other) const;

  /// 计算时间差（秒，浮点数）
  [[nodiscard]] double diffSeconds(const Timestamp &other) const;

  /// 经过的时间（从创建到现在）
  [[nodiscard]] int64_t elapsedNanoseconds() const {
    return now().diffNanoseconds(*this);
  }

  [[nodiscard]] int64_t elapsedMicroseconds() const {
    return now().diffMicroseconds(*this);
  }

  [[nodiscard]] int64_t elapsedMilliseconds() const {
    return now().diffMilliseconds(*this);
  }

  [[nodiscard]] double elapsedSeconds() const {
    return now().diffSeconds(*this);
  }

  // ========================================
  // 比较
  // ========================================

  bool operator==(const Timestamp &other) const {
    return timePoint_ == other.timePoint_;
  }
  bool operator!=(const Timestamp &other) const { return !(*this == other); }
  bool operator<(const Timestamp &other) const {
    return timePoint_ < other.timePoint_;
  }
  bool operator<=(const Timestamp &other) const {
    return timePoint_ <= other.timePoint_;
  }
  bool operator>(const Timestamp &other) const {
    return timePoint_ > other.timePoint_;
  }
  bool operator>=(const Timestamp &other) const {
    return timePoint_ >= other.timePoint_;
  }

  // ========================================
  // 格式化
  // ========================================

  [[nodiscard]] std::string toString() const {
    return std::to_string(toNanoseconds()) + "ns";
  }

private:
  TimePoint timePoint_;
};

/// 秒表（性能计时器）
class Stopwatch {
public:
  /// 开始计时
  void start();

  /// 停止计时
  void stop();

  /// 重置
  void reset();

  /// 重启
  void restart();

  /// 获取经过的时间（纳秒）
  [[nodiscard]] int64_t elapsedNanoseconds() const;

  [[nodiscard]] int64_t elapsedMicroseconds() const {
    return elapsedNanoseconds() / 1000;
  }

  [[nodiscard]] int64_t elapsedMilliseconds() const {
    return elapsedNanoseconds() / 1000000;
  }

  [[nodiscard]] double elapsedSeconds() const {
    return static_cast<double>(elapsedNanoseconds()) / 1e9;
  }

  [[nodiscard]] bool isRunning() const { return running_; }

private:
  bool running_ = false;
  Timestamp startTime_;
  Timestamp endTime_;
};

/// RAII 作用域计时器
class ScopedTimer {
public:
  using Callback = std::function<void(int64_t nanoseconds)>;

  explicit ScopedTimer(Callback callback)
      : callback_(std::move(callback)), start_(Timestamp::now()) {}

  explicit ScopedTimer(const std::string &label = "")
      : label_(label), start_(Timestamp::now()) {}

  ~ScopedTimer() {
    auto elapsed = Timestamp::now().diffNanoseconds(start_);
    if (callback_) {
      callback_(elapsed);
    } else if (!label_.empty()) {
      // 简单输出到标准输出
      std::printf("[%s] Elapsed: %.3f ms\n", label_.c_str(), elapsed / 1e6);
    }
  }

private:
  Callback callback_;
  std::string label_;
  Timestamp start_;
};

} // namespace date
} // namespace darwincore

#endif // DARWINCORE_TIMESTAMP_H
