//
// StringPool.h
// DarwinCore
//
// 设计目标：
// 1. 对重复出现的字符串进行去重存储，降低内存占用
// 2. 将字符串比较从 O(n) 降为 O(1)（指针比较）
// 3. 提供稳定生命周期的 std::string_view，用于高频比较与哈希
//
// 典型使用场景：
// - 配置 / JSON / IPC 的 key
// - 日志字段名、事件类型
// - 安全规则、策略匹配（bundle_id / team_id / path）
// - macOS EndpointSecurity / DLP / 守护进程中的常驻字符串
//
// ⚠️ 注意：
// - StringPool 中的字符串不会自动回收，生命周期等同于 StringPool 本身
// - 不适合存储高基数、低重复率、不可控输入（如 UUID、用户自由文本）
//

#ifndef DARWINCORE_STRING_POOL_H
#define DARWINCORE_STRING_POOL_H

#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_set>

namespace darwincore {
    namespace string {
        /**
     * @brief 字符串池（String Interning Pool）
     *
     * 核心语义：
     * - 对相同内容的字符串，仅在内存中保存一份
     * - intern 后返回的 std::string_view 指向池内稳定内存
     *
     * 性能特性：
     * - intern 查找/插入：O(1) 平均复杂度
     * - 字符串比较：可退化为指针比较（O(1)）
     * - 哈希：基于地址，无需遍历字符内容
     *
     * 线程安全：
     * - intern / contains / size / clear 均通过 mutex 保护
     * - 适合多线程并发读写，但不是无锁结构
     *
     * 生命周期约束：
     * - 所有返回的 string_view 在 StringPool 销毁或 clear() 前始终有效
     * - clear() 会使所有已获取的 string_view 失效（悬空）
     */
        class StringPool {
        public:
            StringPool() = default;

            ~StringPool() = default;

            StringPool(const StringPool &) = delete;

            StringPool &operator=(const StringPool &) = delete;

            /**
             * @brief 获取池化字符串（如不存在则添加）
             */
            std::string_view intern(const std::string &str);

            std::string_view intern(std::string_view str);

            /**
             * @brief 检查字符串是否已池化
             */
            [[nodiscard]] bool contains(const std::string &str) const;

            /**
             * @brief 获取池化字符串数量
             */
            [[nodiscard]] size_t size() const;

            /**
             * @brief 清空池
             */
            void clear();

            /**
             * @brief 获取全局共享实例
             */
            static StringPool &shared() {
                static StringPool instance;
                return instance;
            }

        private:
            mutable std::mutex mutex_;
            std::unordered_set<std::string> pool_;
        };

        /**
 * @brief 池化字符串引用（Interned String Handle）
 *
 * 设计目的：
 * - 作为 std::string / std::string_view 的轻量替代
 * - 提供 O(1) 比较与哈希能力
 *
 * 核心假设：
 * - 所有 InternedString 均来自同一个 StringPool
 * - 相同内容的字符串，其 data() 指针必然相同
 *
 * ⚠️ 注意：
 * - 不持有 StringPool 的所有权
 * - StringPool 被 clear() 或销毁后，本对象失效
 */
        class InternedString {
        public:
            InternedString() : view_() {
            }

            explicit InternedString(const std::string &str,
                                    StringPool &pool = StringPool::shared())
                : view_(pool.intern(str)) {
            }

            [[nodiscard]] std::string_view view() const noexcept { return view_; }
            [[nodiscard]] const char *c_str() const noexcept { return view_.data(); }
            [[nodiscard]] size_t size() const noexcept { return view_.size(); }
            [[nodiscard]] bool empty() const noexcept { return view_.empty(); }

            bool operator==(const InternedString &other) const {
                return view_.data() == other.view_.data(); // 指针比较
            }

            bool operator!=(const InternedString &other) const {
                return !(*this == other);
            }

            bool operator<(const InternedString &other) const {
                return view_.data() < other.view_.data();
            }

        private:
            std::string_view view_;
        };
    } // namespace string
} // namespace darwincore

// std::hash 特化
namespace std {
    template<>
    struct hash<darwincore::string::InternedString> {
        size_t operator()(const darwincore::string::InternedString &s) const {
            return std::hash<const void *>{}(s.view().data());
        }
    };
} // namespace std

#endif // DARWINCORE_STRING_POOL_H
