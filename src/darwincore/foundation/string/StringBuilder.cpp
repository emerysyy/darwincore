//
// StringBuilder.cpp
// DarwinCore
//

#include <darwincore/foundation/string/StringBuilder.h>
#include <cstddef>
#include <string>
#include <string_view>

namespace darwincore {
namespace string {

StringBuilder::StringBuilder(size_t reserveSize) {
  buffer_.reserve(reserveSize);
}

StringBuilder::StringBuilder(const std::string &initial) : buffer_(initial) {}

StringBuilder &StringBuilder::append(const std::string &str) {
  buffer_ += str;
  return *this;
}

StringBuilder &StringBuilder::append(std::string_view str) {
  buffer_.append(str);
  return *this;
}

StringBuilder &StringBuilder::append(const char *str) {
  if (str)
    buffer_ += str;
  return *this;
}

StringBuilder &StringBuilder::append(char c) {
  buffer_ += c;
  return *this;
}

StringBuilder &StringBuilder::append(char c, size_t count) {
  buffer_.append(count, c);
  return *this;
}

StringBuilder &StringBuilder::append(bool value) {
  buffer_ += value ? "true" : "false";
  return *this;
}

StringBuilder &StringBuilder::appendLine(const std::string &str) {
  buffer_ += str;
  buffer_ += '\n';
  return *this;
}

StringBuilder &StringBuilder::insert(size_t pos, const std::string &str) {
  if (pos <= buffer_.size())
    buffer_.insert(pos, str);
  return *this;
}

StringBuilder &StringBuilder::replace(size_t pos, size_t len,
                                      const std::string &str) {
  if (pos <= buffer_.size())
    buffer_.replace(pos, len, str);
  return *this;
}

StringBuilder &StringBuilder::clear() {
  buffer_.clear();
  return *this;
}

StringBuilder &StringBuilder::reserve(size_t size) {
  buffer_.reserve(size);
  return *this;
}

std::string StringBuilder::toString() const { return buffer_; }

std::string_view StringBuilder::toStringView() const { return buffer_; }

const char *StringBuilder::c_str() const { return buffer_.c_str(); }

StringBuilder &StringBuilder::removeLast(size_t count) {
  if (count <= buffer_.size())
    buffer_.erase(buffer_.size() - count);
  return *this;
}

StringBuilder &StringBuilder::trim() {
  size_t start = buffer_.find_first_not_of(" \t\n\r");
  if (start == std::string::npos) {
    buffer_.clear();
    return *this;
  }
  size_t end = buffer_.find_last_not_of(" \t\n\r");
  buffer_ = buffer_.substr(start, end - start + 1);
  return *this;
}

} // namespace string
} // namespace darwincore
