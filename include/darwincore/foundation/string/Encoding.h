//
// Encoding.h
// DarwinCore
//

#ifndef DARWINCORE_ENCODING_H
#define DARWINCORE_ENCODING_H

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace darwincore {
namespace string {

/// 字符编码类型
enum class Encoding {
  UTF8,
  UTF16LE,
  UTF16BE,
  UTF32LE,
  UTF32BE,
  ASCII,
  Latin1,
  GBK,
  GB2312,
  GB18030
};

/// 字符编码转换工具
class EncodingConverter {
public:
  /// UTF-8 转 UTF-16
  static std::u16string utf8ToUtf16(const std::string &utf8);

  /// UTF-16 转 UTF-8
  static std::string utf16ToUtf8(const std::u16string &utf16);

  /// UTF-8 转 UTF-32
  static std::u32string utf8ToUtf32(const std::string &utf8);

  /// UTF-32 转 UTF-8
  static std::string utf32ToUtf8(const std::u32string &utf32);

  /// 检测 UTF-8 是否有效
  static bool isValidUtf8(const std::string &str);

  /// 计算 UTF-8 字符数量
  static size_t utf8CharCount(const std::string &str);

#ifdef __APPLE__
  /// 使用 CoreFoundation 进行编码转换
  static std::optional<std::string> convert(const std::string &str,
                                            Encoding from, Encoding to);
#endif

private:
  static size_t utf8CharLength(unsigned char c);
  static uint32_t decodeUtf8Char(const std::string &str, size_t &i);
  static void encodeUtf8Char(std::string &result, uint32_t cp);

#ifdef __APPLE__
  static CFStringEncoding toCFEncoding(Encoding enc);
#endif
};

} // namespace string
} // namespace darwincore

#endif // DARWINCORE_ENCODING_H
