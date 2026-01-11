//
// Regex.h
// DarwinCore
//

#ifndef DARWINCORE_REGEX_H
#define DARWINCORE_REGEX_H

#include <cstddef>
#include <optional>
#include <regex>
#include <string>
#include <vector>

namespace darwincore {
    namespace string {
        /// 正则表达式匹配结果
        struct RegexMatch {
            std::string value; // 匹配的字符串
            size_t position; // 起始位置
            size_t length; // 长度
            std::vector<std::string> groups; // 捕获组
        };

        /// 正则表达式封装
        class Regex {
        public:
            /// 构造（ECMAScript 语法）
            explicit Regex(
                const std::string &pattern,
                std::regex_constants::syntax_option_type flags = std::regex::ECMAScript)
                : pattern_(pattern), regex_(pattern, flags) {
            }

            /// 检查是否完全匹配
            bool match(const std::string &str) const;

            /// 检查是否包含匹配
            bool search(const std::string &str) const;

            /// 查找第一个匹配
            std::optional<RegexMatch> findFirst(const std::string &str) const;

            /// 查找所有匹配
            std::vector<RegexMatch> findAll(const std::string &str) const;

            /// 替换第一个匹配
            std::string replaceFirst(const std::string &str,
                                     const std::string &replacement) const;

            /// 替换所有匹配
            std::string replaceAll(const std::string &str,
                                   const std::string &replacement) const;

            /// 按正则分割字符串
            std::vector<std::string> split(const std::string &str) const;

            /// 获取原始模式
            const std::string &pattern() const { return pattern_; }

            /// 静态便捷方法
            static bool matches(const std::string &str, const std::string &pattern) {
                return std::regex_match(str, std::regex(pattern));
            }

            static bool contains(const std::string &str, const std::string &pattern) {
                return std::regex_search(str, std::regex(pattern));
            }

            static std::string replace(const std::string &str, const std::string &pattern,
                                       const std::string &replacement) {
                return std::regex_replace(str, std::regex(pattern), replacement);
            }

        private:
            std::string pattern_;
            std::regex regex_;
        };
    } // namespace string
} // namespace darwincore

#endif // DARWINCORE_REGEX_H
