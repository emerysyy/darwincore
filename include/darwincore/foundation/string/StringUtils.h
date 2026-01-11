//
// Created by Darwin Core on 2023/1/17.
//

#ifndef MYLIB_STRINGUTILS_H
#define MYLIB_STRINGUTILS_H

#include <string>
#include <vector>

namespace darwincore {
    namespace string {
        // ========================================
        // 字符串工具类
        // ========================================
        // 功能：提供常用的字符串操作工具方法
        // ========================================
        class StringUtils {
        public:
            // 分割字符串
            // str: 原始字符串
            // separator: 分隔符
            // 返回：分割后的字符串数组
            static std::vector<std::string> split(const std::string &str, const std::string &separator);

            // 连接字符串数组
            // array: 字符串数组
            // separator: 连接符
            // 返回：连接后的字符串
            static std::string join(const std::vector<std::string> &array, const std::string &separator);

            // 检查字符串是否以指定前缀开头
            static bool hasPrefix(const std::string &str, const std::string &prefix);

            // 检查字符串是否以指定后缀结尾
            static bool hasSuffix(const std::string &str, const std::string &suffix);

            // 转换为小写
            static std::string toLowerCase(const std::string &str);

            // 转换为大写
            static std::string toUpperCase(const std::string &str);

            // 格式化字符串（类似 printf）
            static std::string stringWithFormat(const char *fmt, ...);

            // 格式化字符串（va_list 版本）
            static std::string stringWithFormat(const char *fmt, va_list args);

            // 替换字符串中的所有匹配项
            static std::string replace(const std::string &str, const std::string &target, const std::string &with);

            // 将字节缓冲区转换为 UTF-8 字符串
            static std::string byte2utf8string(const char *buffer, size_t size);

            // 转义百分号（将 % 替换为 %%）
            static std::string escapePercent(const std::string& str);

        };
    };
};


#endif //MYLIB_STRINGUTILS_H
