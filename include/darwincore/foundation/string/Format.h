//
// Format.h
// DarwinCore
//

#ifndef DARWINCORE_FORMAT_H
#define DARWINCORE_FORMAT_H

#include <cstdio>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace darwincore {
namespace string {

/// 类型安全的格式化工具
class Format {
public:
  /// 基础格式化（可变参数模板）
  template <typename... Args>
  static std::string format(const std::string &fmt, Args &&...args) {
    std::ostringstream oss;
    formatImpl(oss, fmt, 0, std::forward<Args>(args)...);
    return oss.str();
  }

  /// printf 风格格式化
  template <typename... Args>
  static std::string sprintf(const char *fmt, Args... args) {
    int size = std::snprintf(nullptr, 0, fmt, args...);
    if (size <= 0)
      return "";
    std::string result(size, '\0');
    std::snprintf(&result[0], size + 1, fmt, args...);
    return result;
  }

  /// 数值格式化
  static std::string number(int64_t value, int base = 10) {
    if (base == 16) {
      std::ostringstream oss;
      oss << std::hex << value;
      return oss.str();
    } else if (base == 8) {
      std::ostringstream oss;
      oss << std::oct << value;
      return oss.str();
    }
    return std::to_string(value);
  }

  /// 浮点数格式化
  static std::string decimal(double value, int precision = 2) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << value;
    return oss.str();
  }

  /// 科学计数法
  static std::string scientific(double value, int precision = 2) {
    std::ostringstream oss;
    oss << std::scientific << std::setprecision(precision) << value;
    return oss.str();
  }

  /// 填充字符串
  static std::string pad(const std::string &str, size_t width, char fill = ' ',
                         bool left = true) {
    if (str.size() >= width)
      return str;
    std::string padding(width - str.size(), fill);
    return left ? (padding + str) : (str + padding);
  }

  /// 左对齐
  static std::string leftAlign(const std::string &str, size_t width,
                               char fill = ' ') {
    return pad(str, width, fill, false);
  }

  /// 右对齐
  static std::string rightAlign(const std::string &str, size_t width,
                                char fill = ' ') {
    return pad(str, width, fill, true);
  }

  /// 居中对齐
  static std::string center(const std::string &str, size_t width,
                            char fill = ' ') {
    if (str.size() >= width)
      return str;
    size_t padding = width - str.size();
    size_t leftPad = padding / 2;
    size_t rightPad = padding - leftPad;
    return std::string(leftPad, fill) + str + std::string(rightPad, fill);
  }

  /// 十六进制格式化
  static std::string hex(uint64_t value, bool uppercase = true,
                         bool prefix = true) {
    std::ostringstream oss;
    if (prefix)
      oss << "0x";
    oss << (uppercase ? std::uppercase : std::nouppercase) << std::hex << value;
    return oss.str();
  }

  /// 字节大小格式化
  static std::string bytes(uint64_t size, int precision = 2) {
    const char *units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    int unit = 0;
    double displaySize = static_cast<double>(size);
    while (displaySize >= 1024 && unit < 5) {
      displaySize /= 1024;
      ++unit;
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(unit == 0 ? 0 : precision)
        << displaySize << " " << units[unit];
    return oss.str();
  }

  /// 百分比格式化
  static std::string percent(double value, int precision = 1) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << (value * 100) << "%";
    return oss.str();
  }

private:
  // 格式化实现（递归终止）
  static void formatImpl(std::ostringstream &oss, const std::string &fmt,
                         size_t pos) {
    while (pos < fmt.size()) {
      if (fmt[pos] == '{' && pos + 1 < fmt.size() && fmt[pos + 1] == '}') {
        throw std::runtime_error("Format: not enough arguments");
      }
      oss << fmt[pos++];
    }
  }

  // 格式化实现（递归展开）
  template <typename T, typename... Rest>
  static void formatImpl(std::ostringstream &oss, const std::string &fmt,
                         size_t pos, T &&value, Rest &&...rest) {
    while (pos < fmt.size()) {
      if (fmt[pos] == '{' && pos + 1 < fmt.size() && fmt[pos + 1] == '}') {
        oss << value;
        formatImpl(oss, fmt, pos + 2, std::forward<Rest>(rest)...);
        return;
      }
      oss << fmt[pos++];
    }
  }
};

} // namespace string
} // namespace darwincore

#endif // DARWINCORE_FORMAT_H
