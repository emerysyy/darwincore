//
// StringBuilder.h
// DarwinCore
//

#ifndef DARWINCORE_STRING_BUILDER_H
#define DARWINCORE_STRING_BUILDER_H

#include <cstdio>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace darwincore {
    namespace string {
        /// 高效字符串构建器
        class StringBuilder {
        public:
            StringBuilder() = default;

            explicit StringBuilder(size_t reserveSize);

            explicit StringBuilder(const std::string &initial);

            /// 追加字符串
            StringBuilder &append(const std::string &str);

            StringBuilder &append(std::string_view str);

            StringBuilder &append(const char *str);

            StringBuilder &append(char c);

            StringBuilder &append(char c, size_t count);

            /// 追加数值
            template<typename T>
            std::enable_if_t<std::is_arithmetic_v<T>, StringBuilder &> append(T value) {
                buffer_ += std::to_string(value);
                return *this;
            }

            /// 追加布尔值
            StringBuilder &append(bool value);

            /// 流式操作符
            template<typename T>
            StringBuilder &operator<<(const T &value) {
                return append(value);
            }

            /// 追加换行
            StringBuilder &appendLine(const std::string &str = "");

            /// 格式化追加（printf 风格）
            template<typename... Args>
            StringBuilder &appendFormat(const char *fmt, Args... args) {
                int size = std::snprintf(nullptr, 0, fmt, args...);
                if (size > 0) {
                    size_t oldSize = buffer_.size();
                    buffer_.resize(oldSize + size + 1);
                    std::snprintf(&buffer_[oldSize], size + 1, fmt, args...);
                    buffer_.resize(oldSize + size); // 移除 null 终止符
                }
                return *this;
            }

            /// 插入字符串
            StringBuilder &insert(size_t pos, const std::string &str);

            /// 替换
            StringBuilder &replace(size_t pos, size_t len, const std::string &str);

            /// 清空
            StringBuilder &clear();

            /// 预留空间
            StringBuilder &reserve(size_t size);

            /// 获取结果
            [[nodiscard]] std::string toString() const;

            [[nodiscard]] std::string_view toStringView() const;

            [[nodiscard]] const char *c_str() const;

            /// 使用分隔符连接
            template<typename Container>
            StringBuilder &join(const Container &items, const std::string &separator) {
                bool first = true;
                for (const auto &item: items) {
                    if (!first)
                        buffer_ += separator;
                    first = false;
                    append(item);
                }
                return *this;
            }

            /// 获取长度
            [[nodiscard]] size_t length() const noexcept { return buffer_.size(); }
            [[nodiscard]] size_t size() const noexcept { return buffer_.size(); }
            [[nodiscard]] bool empty() const noexcept { return buffer_.empty(); }

            /// 删除末尾字符
            StringBuilder &removeLast(size_t count = 1);

            /// 去除首尾空白
            StringBuilder &trim();

        private:
            std::string buffer_;
        };
    } // namespace string
} // namespace darwincore

#endif // DARWINCORE_STRING_BUILDER_H
