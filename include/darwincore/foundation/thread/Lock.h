//
// Created by Darwin Core on 2024/3/27.
//

#ifndef __LOCK_H__
#define __LOCK_H__

#include <mutex>
#include <condition_variable>
#include <atomic>

namespace darwincore {
    namespace thread {

        // ========================================
        // 信号量类
        // ========================================
        // 功能：
        // 1. 实现经典的信号量同步机制
        // 2. 支持 post（增加信号量）和 wait（减少信号量）操作
        // 3. 支持 wait 超时机制
        // ========================================
        class Semaphore {
        public:
            explicit Semaphore(int count = 1);  // 初始化信号量计数，默认为 1（二值信号量）
            ~Semaphore();

        public:
            // 增加信号量计数（V 操作），唤醒一个等待线程
            void post();

            // 减少信号量计数（P 操作）
            // for_ms: 超时时间（毫秒），0 表示无限等待
            // 返回 false 表示超时
            bool wait(int for_ms = 0);

        private:
            int     _count;                       // 信号量计数
            std::mutex  _mutex;                   // 保护 _count 的互斥锁
            std::condition_variable _cv;         // 条件变量，用于等待和唤醒
        };


        // ========================================
        // 读写锁类
        // ========================================
        // 功能：
        // 1. 实现读者-写者问题的解决方案
        // 2. 支持多个读者同时读取（读读共享）
        // 3. 写者互斥访问，读写互斥（写写互斥、读写互斥）
        // 4. 采用写者优先策略
        // ========================================
        class ReadWriteLock {
        public:
            ReadWriteLock() ;
            ~ReadWriteLock();
            // 禁止拷贝和赋值
            ReadWriteLock& operator=(const ReadWriteLock&) = delete;
            ReadWriteLock(const ReadWriteLock&) = delete;

            // 获取写锁（独占锁）
            void lockWrite();

            // 释放写锁
            void unlockWrite();

            // 获取读锁（共享锁）
            void lockRead();

            // 释放读锁
            void unlockRead();

        private:
            std::mutex _mutex;                    // 保护内部状态的互斥锁
            std::condition_variable _cv_writer;  // 写者等待条件变量
            std::condition_variable _cv_reader;  // 读者等待条件变量
            int _waitWriters;                    // 等待的写者数量
            int _readers;                         // 当前读者数量
            int _writers;                         // 当前写者数量（0 或 1）
        };
    };
};


#endif //__LOCK_H__
