//
// MMap.h
// DarwinCore
//

#ifndef DARWINCORE_MMAP_H
#define DARWINCORE_MMAP_H

#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace darwincore {
    namespace file {
        /// 内存映射模式（直接映射到 mmap / open 语义）
        ///
        /// 注意：
        /// - mmap 并不会“读取文件内容到内存”，
        ///   只是建立「文件 ↔ 进程虚拟地址空间」的映射关系
        /// - 实际数据在首次访问对应页面时，由内核按需加载（page fault）
        enum class MMapMode {
            ///< 只读映射（PROT_READ）
            ///< - 文件内容不可修改
            ///< - 写入将导致 SIGBUS
            ReadOnly,

            ///< 读写映射（PROT_READ | PROT_WRITE, MAP_SHARED）
            ///< - 修改会反映到文件
            ///< - 需显式调用 sync() 才能保证落盘
            ReadWrite,

            ///< 写时复制（PROT_READ | PROT_WRITE, MAP_PRIVATE）
            ///< - 修改只对当前进程可见
            ///< - 不会写回原文件
            CopyOnWrite
        };


        /// mmap 文件封装（RAII）
        ///
        /// 设计目标：
        /// - 提供零拷贝文件访问能力
        /// - 将文件内容视为一段只读 / 可写的内存
        /// - 自动管理 mmap / munmap / close 生命周期
        ///
        /// 重要语义说明（必须理解）：
        ///
        /// 1. mmap != 读文件
        ///    mmap 只是建立映射，真正的数据读取发生在访问内存时
        ///
        /// 2. data() / as<T>() / view<T>() 只是“指针解释”
        ///    - 不做 memcpy
        ///    - 不做构造
        ///    - 不做边界检查（除非显式提供）
        ///
        /// 3. 调用者必须保证：
        ///    - 文件大小 >= 访问的数据结构大小
        ///    - 文件布局与结构体 ABI 完全一致
        ///    - 对齐、大小端等问题已处理
        ///
        /// 4. 适合：
        ///    - 大文件
        ///    - 只读配置 / 索引 / 规则文件
        ///    - 顺序或随机扫描
        ///
        /// 5. 不适合：
        ///    - 小文件
        ///    - 高频细粒度写入
        ///    - 需要强事务一致性的场景

        class MMap {
        public:
            MMap() = default;

            /// 析构时自动解除映射并关闭文件
            ~MMap() { unmap(); }

            /// 禁止拷贝（避免 double munmap / close）
            MMap(const MMap &) = delete;

            MMap &operator=(const MMap &) = delete;

            /// 支持移动语义
            /// - 转移 mmap 资源所有权
            /// - 被移动对象会被置为“未映射状态”
            MMap(MMap &&other) noexcept
                : data_(other.data_), size_(other.size_), fd_(other.fd_) {
                other.data_ = nullptr;
                other.size_ = 0;
                other.fd_ = -1;
            }

            MMap &operator=(MMap &&other) noexcept {
                if (this != &other) {
                    unmap();
                    data_ = other.data_;
                    size_ = other.size_;
                    fd_ = other.fd_;
                    other.data_ = nullptr;
                    other.size_ = 0;
                    other.fd_ = -1;
                }
                return *this;
            }

            /// 映射文件到当前进程地址空间
            ///
            /// @param path 文件路径
            /// @param mode 映射模式（只读 / 读写 / 写时复制）
            ///
            /// @return 是否映射成功
            ///
            /// 失败常见原因：
            /// - 文件不存在或无权限
            /// - 文件大小为 0
            /// - mmap 返回 MAP_FAILED
            ///
            /// 注意：
            /// - fd 在整个映射期间必须保持打开
            /// - 文件大小在映射期间不应被外部 truncate
            bool map(const std::string &path, MMapMode mode = MMapMode::ReadOnly);

            /// 解除映射
            ///
            /// - munmap(data_, size_)
            /// - close(fd_)
            /// - 重置内部状态
            ///
            /// 可安全重复调用
            void unmap();

            /// 将映射内容同步到磁盘（仅对 ReadWrite 模式有意义）
            ///
            /// @param async
            ///   - false: MS_SYNC  （阻塞，保证写入完成）
            ///   - true : MS_ASYNC （异步，内核后台写回）
            ///
            /// 注意：
            /// - CopyOnWrite 模式下调用无实际意义
            /// - 不调用 sync() 不保证数据落盘
            bool sync(bool async = false);

            /// 获取映射区起始地址
            ///
            /// 返回的是 mmap 返回的虚拟地址
            /// - 不进行任何类型转换
            /// - 不保证对齐
            void *data() { return data_; }
            const void *data() const { return data_; }

            /// 将映射区起始地址解释为 T*
            ///
            /// ⚠️ 低级接口，仅做指针类型转换（reinterpret）
            ///
            /// 语义说明：
            /// - 不读取、不拷贝、不构造
            /// - 不做任何边界检查
            /// - 不检查对齐、大小端或 ABI
            ///
            /// 调用者必须保证：
            /// - size() >= sizeof(T)
            /// - 文件布局与 T 完全一致
            /// - 访问不会越界，否则可能触发 SIGBUS
            ///
            /// 适合：
            /// - 明确知道文件格式且追求极致性能的场景
            template<typename T>
            T *as() {
                return static_cast<T *>(data_);
            }

            template<typename T>
            const T *as() const {
                return static_cast<const T *>(data_);
            }

            /// 从指定偏移处，将映射区内容解释为只读 T 视图
            ///
            /// 语义说明：
            /// - 这是“内存视图（view）”，不是读取操作
            /// - 仅做地址计算：data_ + offset
            /// - 不进行 memcpy / 构造 / 初始化
            /// - 不维护任何内部游标或状态
            ///
            /// 安全性：
            /// - 会检查 offset + sizeof(T) 是否越界
            /// - 越界时返回 nullptr（避免 SIGBUS）
            ///
            /// offset 语义：
            /// - offset 完全由调用者控制
            /// - 本函数不会自动推进 offset
            ///
            /// 使用前提：
            /// - 文件内容布局必须与 T 的内存布局完全一致
            /// - 调用者需自行处理对齐、大小端、ABI 等问题
            ///
            /// 适用场景：
            /// - mmap 文件解析
            /// - 索引 / 配置 / 规则等只读数据访问
            ///
            /// @tparam T  目标类型（通常为 POD / trivial 类型）
            /// @param offset  相对于映射起始地址的字节偏移
            /// @return
            ///   - 成功：指向映射区内的只读 T*
            ///   - 失败：nullptr（未映射或越界）
            template<typename T>
            const T *view(size_t offset = 0) const {
                if (!data_ || offset + sizeof(T) > size_) {
                    return nullptr;
                }

                return reinterpret_cast<const T *>(
                    static_cast<const uint8_t *>(data_) + offset);
            }

            /// 从指定偏移处，将映射区内容解释为可写 T 视图
            ///
            /// ⚠️ 注意：
            /// - 该接口仅在 ReadWrite / CopyOnWrite 映射模式下合法
            /// - 在 ReadOnly 映射下将返回 nullptr
            ///
            /// 语义说明：
            /// - 这是“内存视图（view）”，不是读取或写入操作
            /// - 返回的指针直接指向 mmap 映射区
            /// - 对该指针的写操作等价于修改映射内存
            ///
            /// 安全性：
            /// - 会检查 offset + sizeof(T) 是否越界
            /// - 会检查当前映射是否允许写入
            ///
            /// offset 语义：
            /// - offset 完全由调用者控制
            /// - 本函数不会自动推进 offset
            ///
            /// 写入行为说明：
            /// - ReadWrite  : 修改会反映到文件（需 sync() 保证落盘）
            /// - CopyOnWrite: 修改仅对当前进程可见
            ///
            /// 使用前提：
            /// - 文件布局与 T 完全一致
            /// - 调用者需保证写入不会破坏文件结构
            ///
            /// @tparam T  目标类型（通常为 POD / trivial 类型）
            /// @param offset  相对于映射起始地址的字节偏移
            /// @return
            ///   - 成功：指向映射区内的可写 T*
            ///   - 失败：nullptr（未映射 / 越界 / 当前模式只读）
            template<typename T>
            T *view(size_t offset = 0) {
                if (!data_ || offset + sizeof(T) > size_) {
                    return nullptr;
                }

                // 只读映射不允许返回可写指针
                if (mode_ == MMapMode::ReadOnly) {
                    return nullptr;
                }

                return reinterpret_cast<T *>(
                    static_cast<uint8_t *>(data_) + offset);
            }


            /// 获取大小
            size_t size() const { return size_; }

            /// 是否已映射
            bool isMapped() const { return data_ != nullptr; }

            /// 预取数据到内存
            /// 提示内核提前加载映射页面（madvise: MADV_WILLNEED）
            ///
            /// 适合：
            /// - 顺序扫描
            /// - 避免首次访问 page fault
            void prefetch();

            /// 提示内核该映射主要顺序访问（MADV_SEQUENTIAL）
            ///
            /// - 有助于 readahead
            /// - 可能影响页缓存淘汰策略
            void setSequential();

            /// 标记为随机访问
            /// 提示内核该映射主要随机访问（MADV_RANDOM）
            ///
            /// - 抑制无效 readahead
            /// - 适合索引 / 哈希访问
            void setRandom();

        private:
            void *data_ = nullptr;
            size_t size_ = 0;
            int fd_ = -1;
            MMapMode mode_ = MMapMode::ReadOnly;
        };
    } // namespace file
} // namespace darwincore

#endif // DARWINCORE_MMAP_H
