//
// Created by 李培 on 2024/10/20.
//

#ifndef SHAREDMEMORY_H
#define SHAREDMEMORY_H

#include <iostream>

namespace darwincore {
    namespace process {

        // ========================================
        // 进程间信号量类
        // ========================================
        // 功能：
        // 1. 提供基于命名信号量的进程间同步机制
        // 2. 使用 POSIX sem_open 实现
        // 3. 支持超时等待
        // ========================================
        class Semaphore {

        public:
            // 构造函数
            // path: 信号量名称（必须以 "/" 开头，如 "/mysem"）
            explicit Semaphore(std::string path);
            ~Semaphore();


        public:
            // 初始化信号量
            // offset: 信号量在共享内存中的偏移量（预留参数，当前未使用）
            // value: 信号量的初始值，默认为 0
            // 返回：true 表示初始化成功，false 表示失败
            bool initialize(int offset, int value = 0);

            // 销毁信号量
            void uninitialize();

            // 等待信号量（P 操作）
            // timeout_ms: 超时时间（毫秒），0 表示无限等待
            // 返回：0 表示成功，-1 表示失败或超时
            int wait(int timeout_ms = 0);

            // 释放信号量（V 操作）
            // 返回：0 表示成功，-1 表示失败
            int post();

        private:
            std::string _path;      // 信号量名称
            int _sem_id;         // 信号量句柄
        };


        // ========================================
        // 共享内存类
        // ========================================
        // 功能：
        // 1. 提供进程间共享内存机制
        // 2. 使用 POSIX shm_open 和 mmap 实现
        // 3. 内置两个信号量用于同步访问
        // ========================================
        class SharedMemory {

        public:
            // 构造函数
            // path: 共享内存名称（必须以 "/" 开头，如 "/myshm"）
            explicit SharedMemory(std::string path);
            ~SharedMemory();

        public:
            // 打开共享内存（内部调用 getSharedMemory）
            // 返回：true 表示成功，false 表示失败
            bool open();

            // 获取或创建共享内存
            // create: 是否创建新的共享内存（true=创建，false=只打开已有）
            // 返回：true 表示成功，false 表示失败
            bool getSharedMemory(bool create = false);

            // 销毁共享内存
            // 返回：true 表示成功，false 表示失败
            bool destroySharedMemory();

        private:
            std::string _path;           // 共享内存名称
            int _shm_id;                 // 共享内存文件描述符
            void *_shm_ptr;              // 共享内存映射指针
            Semaphore _shm_sem;          // 用于通知读者（数据就绪信号量）
            Semaphore _shm_mutex;       // 用于锁住共享内存（互斥信号量）

        };
    };
};


#endif //SHAREDMEMORY_H
