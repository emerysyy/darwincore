//
// Timestamp.cpp
// DarwinCore
//

#include <darwincore/foundation/date/Timestamp.h>

namespace darwincore {
namespace date {

int64_t Timestamp::diffNanoseconds(const Timestamp &other) const {
  return std::chrono::duration_cast<Nanoseconds>(timePoint_ - other.timePoint_)
      .count();
}

int64_t Timestamp::diffMicroseconds(const Timestamp &other) const {
  return std::chrono::duration_cast<std::chrono::microseconds>(timePoint_ -
                                                               other.timePoint_)
      .count();
}

int64_t Timestamp::diffMilliseconds(const Timestamp &other) const {
  return std::chrono::duration_cast<std::chrono::milliseconds>(timePoint_ -
                                                               other.timePoint_)
      .count();
}

double Timestamp::diffSeconds(const Timestamp &other) const {
  return std::chrono::duration<double>(timePoint_ - other.timePoint_).count();
}

void Stopwatch::start() {
  running_ = true;
  startTime_ = Timestamp::now();
}

void Stopwatch::stop() {
  if (running_) {
    endTime_ = Timestamp::now();
    running_ = false;
  }
}

void Stopwatch::reset() {
  running_ = false;
  startTime_ = Timestamp();
  endTime_ = Timestamp();
}

void Stopwatch::restart() {
  reset();
  start();
}

int64_t Stopwatch::elapsedNanoseconds() const {
  if (running_)
    return Timestamp::now().diffNanoseconds(startTime_);
  return endTime_.diffNanoseconds(startTime_);
}

} // namespace date
} // namespace darwincore
