//
// Encoding.cpp
// DarwinCore
//

#include <darwincore/foundation/string/Encoding.h>

namespace darwincore {
namespace string {

std::u16string EncodingConverter::utf8ToUtf16(const std::string &utf8) {
  std::u16string result;
  size_t i = 0;
  while (i < utf8.size()) {
    uint32_t codepoint = 0;
    size_t len = utf8CharLength(utf8[i]);

    if (len == 1) {
      codepoint = (unsigned char)utf8[i];
    } else if (len == 2 && i + 1 < utf8.size()) {
      codepoint = ((utf8[i] & 0x1F) << 6) | (utf8[i + 1] & 0x3F);
    } else if (len == 3 && i + 2 < utf8.size()) {
      codepoint = ((utf8[i] & 0x0F) << 12) | ((utf8[i + 1] & 0x3F) << 6) |
                  (utf8[i + 2] & 0x3F);
    } else if (len == 4 && i + 3 < utf8.size()) {
      codepoint = ((utf8[i] & 0x07) << 18) | ((utf8[i + 1] & 0x3F) << 12) |
                  ((utf8[i + 2] & 0x3F) << 6) | (utf8[i + 3] & 0x3F);
    } else {
      i++;
      continue;
    }

    if (codepoint <= 0xFFFF) {
      result.push_back(static_cast<char16_t>(codepoint));
    } else {
      codepoint -= 0x10000;
      result.push_back(static_cast<char16_t>(0xD800 + (codepoint >> 10)));
      result.push_back(static_cast<char16_t>(0xDC00 + (codepoint & 0x3FF)));
    }
    i += len;
  }
  return result;
}

std::string EncodingConverter::utf16ToUtf8(const std::u16string &utf16) {
  std::string result;
  size_t i = 0;
  while (i < utf16.size()) {
    uint32_t codepoint = utf16[i];

    if (codepoint >= 0xD800 && codepoint <= 0xDBFF && i + 1 < utf16.size()) {
      uint16_t low = utf16[i + 1];
      if (low >= 0xDC00 && low <= 0xDFFF) {
        codepoint = 0x10000 + ((codepoint - 0xD800) << 10) + (low - 0xDC00);
        ++i;
      }
    }

    encodeUtf8Char(result, codepoint);
    ++i;
  }
  return result;
}

std::u32string EncodingConverter::utf8ToUtf32(const std::string &utf8) {
  std::u32string result;
  size_t i = 0;
  while (i < utf8.size()) {
    uint32_t cp = decodeUtf8Char(utf8, i);
    result.push_back(cp);
  }
  return result;
}

std::string EncodingConverter::utf32ToUtf8(const std::u32string &utf32) {
  std::string result;
  for (char32_t cp : utf32) {
    encodeUtf8Char(result, cp);
  }
  return result;
}

bool EncodingConverter::isValidUtf8(const std::string &str) {
  size_t i = 0;
  while (i < str.size()) {
    size_t len = utf8CharLength(str[i]);
    if (len == 0 || i + len > str.size())
      return false;
    for (size_t j = 1; j < len; ++j) {
      if ((str[i + j] & 0xC0) != 0x80)
        return false;
    }
    i += len;
  }
  return true;
}

size_t EncodingConverter::utf8CharCount(const std::string &str) {
  size_t count = 0;
  for (size_t i = 0; i < str.size();) {
    size_t len = utf8CharLength(str[i]);
    if (len == 0)
      break;
    i += len;
    ++count;
  }
  return count;
}

#ifdef __APPLE__
std::optional<std::string>
EncodingConverter::convert(const std::string &str, Encoding from, Encoding to) {
  CFStringEncoding cfFrom = toCFEncoding(from);
  CFStringEncoding cfTo = toCFEncoding(to);

  CFStringRef cfStr = CFStringCreateWithBytes(
      nullptr, reinterpret_cast<const UInt8 *>(str.data()), str.size(), cfFrom,
      false);
  if (!cfStr)
    return std::nullopt;

  CFIndex length = CFStringGetLength(cfStr);
  CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, cfTo);

  std::string result(maxSize, '\0');
  CFIndex usedSize = 0;
  CFStringGetBytes(cfStr, CFRangeMake(0, length), cfTo, '?', false,
                   reinterpret_cast<UInt8 *>(&result[0]), maxSize, &usedSize);
  CFRelease(cfStr);

  result.resize(usedSize);
  return result;
}
#endif

size_t EncodingConverter::utf8CharLength(unsigned char c) {
  if ((c & 0x80) == 0)
    return 1;
  if ((c & 0xE0) == 0xC0)
    return 2;
  if ((c & 0xF0) == 0xE0)
    return 3;
  if ((c & 0xF8) == 0xF0)
    return 4;
  return 0;
}

uint32_t EncodingConverter::decodeUtf8Char(const std::string &str, size_t &i) {
  size_t len = utf8CharLength(str[i]);
  uint32_t cp = 0;
  switch (len) {
  case 1:
    cp = (unsigned char)str[i];
    break;
  case 2:
    cp = ((str[i] & 0x1F) << 6) | (str[i + 1] & 0x3F);
    break;
  case 3:
    cp = ((str[i] & 0x0F) << 12) | ((str[i + 1] & 0x3F) << 6) |
         (str[i + 2] & 0x3F);
    break;
  case 4:
    cp = ((str[i] & 0x07) << 18) | ((str[i + 1] & 0x3F) << 12) |
         ((str[i + 2] & 0x3F) << 6) | (str[i + 3] & 0x3F);
    break;
  }
  i += len;
  return cp;
}

void EncodingConverter::encodeUtf8Char(std::string &result, uint32_t cp) {
  if (cp < 0x80) {
    result.push_back(static_cast<char>(cp));
  } else if (cp < 0x800) {
    result.push_back(static_cast<char>(0xC0 | (cp >> 6)));
    result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  } else if (cp < 0x10000) {
    result.push_back(static_cast<char>(0xE0 | (cp >> 12)));
    result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
    result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  } else {
    result.push_back(static_cast<char>(0xF0 | (cp >> 18)));
    result.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
    result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
    result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  }
}

#ifdef __APPLE__
CFStringEncoding EncodingConverter::toCFEncoding(Encoding enc) {
  switch (enc) {
  case Encoding::UTF8:
    return kCFStringEncodingUTF8;
  case Encoding::UTF16LE:
    return kCFStringEncodingUTF16LE;
  case Encoding::UTF16BE:
    return kCFStringEncodingUTF16BE;
  case Encoding::ASCII:
    return kCFStringEncodingASCII;
  case Encoding::Latin1:
    return kCFStringEncodingISOLatin1;
  case Encoding::GBK:
    return kCFStringEncodingGB_18030_2000;
  default:
    return kCFStringEncodingUTF8;
  }
}
#endif

} // namespace string
} // namespace darwincore
