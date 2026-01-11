//
// Created by Darwin Core on 2023/1/17.
//

#include <darwincore/foundation/string/StringUtils.h>
#include <sstream>

using namespace darwincore::string;

std::vector<std::string> StringUtils::split(const std::string &str, const std::string &separator) {
    std::vector<std::string> result;
    do {
        if (separator.empty() || str.empty()) {
            break;
        }

        std::string tmpStr(str);
        while (true) {
            auto iter = tmpStr.find_first_of(separator);
            if (iter == std::string::npos) {
                result.push_back(tmpStr);
                break;
            }

            auto substr = tmpStr.substr(0, iter);
            if (!substr.empty()) {
                result.push_back(substr);
            }

            auto nextStart = iter + separator.size();
            tmpStr = tmpStr.substr(nextStart, tmpStr.size() - nextStart);
            if (tmpStr.empty()) {
                break;
            }
        }


    } while (false);

    return result;
}

std::string StringUtils::join(const std::vector<std::string> &array, const std::string &separator) {
    std::string result;
    if (array.empty()) {
        return result;
    }

    std::stringstream ss;
    auto size = array.size();
    for (int i = 0; i < size - 1; ++i) {
        auto s = array[i];
//        result = result + s + separator;
        ss << s << separator;
    }
//    result = result + array[size-1];
    ss << array[size-1];

    return ss.str();;
}

bool StringUtils::hasPrefix(const std::string &str, const std::string &prefix) {

    if (str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0) {
        return true;
    }

    return false;
}

bool StringUtils::hasSuffix(const std::string &str, const std::string &suffix) {

    if (str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0) {
        return true;
    }

    return false;
}

std::string StringUtils::toLowerCase(const std::string &str) {
    std::string ret(str);
    if (str.empty()) {
        return ret;
    }

    std::transform(ret.begin(), ret.end(), ret.begin(), ::tolower);
    return ret;
}

std::string darwincore::string::StringUtils::toUpperCase(const std::string &str) {
    std::string ret(str);
    if (str.empty()) {
        return ret;
    }

    std::transform(ret.begin(), ret.end(), ret.begin(), ::toupper);
    return ret;
}

std::string StringUtils::stringWithFormat(const char *fmt, ...) {
    do {
        if (fmt == nullptr) {
            break;
        }

        int len = 0;
        va_list  args;
        va_start(args, fmt);

        va_list args_copy;
        va_copy(args_copy, args);
        len = vsnprintf(nullptr, 0, fmt, args_copy);
        va_end(args_copy);

        if (len <= 0) {
            va_end(args);
            break;
        }
        len += 1;
        char *buf = (char *)malloc(len);
        if (buf == nullptr) {
            va_end(args);
            break;
        }
        memset(buf, 0, len);

//        va_start(args, fmt);
        vsnprintf(buf, len, fmt, args);
        va_end(args);
        std::string  retStr;
        retStr.assign(buf);

        free(buf);
        return retStr;

    } while(false);
    return "";
}

std::string StringUtils::stringWithFormat(const char *fmt, va_list args) {

    do {
        if (fmt == nullptr) {
            break;
        }

        int len = 0;

        va_list args_copy;
        va_copy(args_copy, args);
        len = vsnprintf(nullptr, 0, fmt, args_copy);
        va_end(args_copy);

        if (len <= 0) {
            break;
        }
        len += 1;
        char *buf = (char *)malloc(len);
        if (buf == nullptr) {
            break;
        }
        memset(buf, 0, len);

        vsnprintf(buf, len, fmt, args);
        std::string  retStr;
        retStr.assign(buf);

        free(buf);
        return retStr;

    } while(false);
    return "";
}


std::string StringUtils::replace(const std::string &str, const std::string &target, const std::string &with) {
    if (str.empty() || target.empty() || with.empty()) {
        return str;
    }

    std::string  ret;
    auto begin= 0;
    auto pos = str.find(target, begin);
    while (pos != std::string::npos) {
        ret.append(str, begin, pos - begin);
        ret += with;
        begin = pos + target.size();
        pos = str.find(target, begin);
    }
    if (begin < str.size()) {
        ret.append(str, begin, str.size() - begin);
    }

    return ret;
}

std::string StringUtils::byte2utf8string(const char *buffer, size_t size) {
    std::string utf8String;
    size_t i = 0;
    while (i < size) {
        unsigned char ch = static_cast<unsigned char>(buffer[i]);
        if (ch < 0x80) {
            utf8String.push_back(static_cast<char>(ch));
            i++;
        } else if (ch < 0xE0) {
            if (i + 1 < size) {
                utf8String.push_back(static_cast<char>((ch >> 6) | 0xC0));
                utf8String.push_back(static_cast<char>((ch & 0x3F) | 0x80));
            }
            i += 2;
        } else if (ch < 0xF0) {
            if (i + 2 < size) {
                utf8String.push_back(static_cast<char>((ch >> 12) | 0xE0));
                utf8String.push_back(static_cast<char>(((ch >> 6) & 0x3F) | 0x80));
                utf8String.push_back(static_cast<char>((ch & 0x3F) | 0x80));
            }
            i += 3;
        } else {
            // Unsupported character
            i++;
        }
    }
    return utf8String;
}

std::string StringUtils::escapePercent(const std::string &str) {
    std::string result;
    size_t pos = 0;
    while (pos < str.length()) {
        if (str[pos] == '%') {
            result += "%%";
        }
        else {
            result += str[pos];
        }
        pos++;
    }

    return result;
}
