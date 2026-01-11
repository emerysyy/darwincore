//
// Created by 李培 on 2026/1/7.
//

#ifndef MEMORYALLOCATOR_H
#define MEMORYALLOCATOR_H

#include <cstddef>
#include <iostream>
#include <new>
#include <utility>

namespace darwincore {
    namespace memory {
        // ========================================
        // 内存块结构
        // ========================================
        // 用于内存池的空闲链表节点
        // ========================================
        struct Block {
            Block *next; // 指向下一个空闲块
        };

        // ========================================
        // 内存池类
        // ========================================
        // 功能：
        // 1. 使用空闲链表管理小块内存，提高内存分配效率
        // 2. 支持内存对齐（8 字节对齐）
        // 3. 大块内存（> 128 字节）直接从系统分配
        // ========================================
        class MemoryPool {
        private:
            enum { ALIGN = 8, MAX_BLOCK = 128 };

            static Block *free_lists[MAX_BLOCK / ALIGN]; // 自由链表数组（16 个链表）

        public:
            // 分配内存
            // size: 需要分配的字节数
            // 返回：分配的内存指针
            static void *allocate(std::size_t size);

            // 释放内存
            // p: 要释放的内存指针
            // size: 原始分配的字节数
            static void deallocate(void *p, std::size_t size);

            // 填充空闲链表
            // 从系统申请大块内存并分割为多个小块加入链表
            static void refill(std::size_t index);
        };

        // ========================================
        // 内存分配器模板类（兼容 STL）
        // ========================================
        // 功能：
        // 1. 实现标准的 STL 分配器接口
        // 2. 使用内存池提高分配效率
        // 3. 支持自定义类型和容器
        // ========================================
        template<typename T>
        class MemoryAllocator {
        public:
            using value_type = T;

            MemoryAllocator() = default;

            // 拷贝构造函数（从其他类型的分配器拷贝）
            template<typename U>
            MemoryAllocator(const MemoryAllocator<U> &) noexcept {
            }

            // 分配 n 个 T 类型的对象
            T *allocate(std::size_t n) {
                if (n > max_size())
                    throw std::bad_alloc();
                // 从内存池分配对齐的内存块（例如 8 字节对齐）
                return static_cast<T *>(MemoryPool::allocate(n * sizeof(T)));
            }

            // 释放内存
            void deallocate(T *p, std::size_t n) noexcept {
                MemoryPool::deallocate(p, n * sizeof(T));
            }

            // 构造对象（可选）
            template<typename U, typename... Args>
            void construct(U *p, Args &&... args) {
                new(p) U(std::forward<Args>(args)...);
            }

            // 销毁对象（可选）
            template<typename U>
            void destroy(U *p) noexcept { p->~U(); }

        private:
            // 返回最大可分配数量
            static std::size_t max_size() noexcept { return 1024; }
        };
    }; // namespace memory
}; // namespace darwincore

#endif // MEMORYALLOCATOR_H
