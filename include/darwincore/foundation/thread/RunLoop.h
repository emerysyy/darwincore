//
// Created by Darwin Core on 2024/5/25.
//

#ifndef RUNLOOP_H
#define RUNLOOP_H

#include <iostream>
#include <condition_variable>
#include <mutex>

namespace darwincore {
    namespace thread {
        // ========================================
        // 运行循环类
        // ========================================
        // 功能：
        // 1. 提供一个可阻塞的运行循环
        // 2. 调用 run() 会阻塞线程，直到调用 stop()
        // 3. 使用条件变量实现线程同步
        // 用途：保持线程运行状态，等待外部信号唤醒
        // ========================================
        class RunLoop {
        public:
            RunLoop();
            ~RunLoop();

            // 进入运行循环，阻塞线程
            // 线程会一直阻塞，直到调用 stop() 方法
            void run();

            // 停止运行循环，唤醒所有等待的线程
            void stop();

        private:
            std::condition_variable _cv;   // 条件变量，用于线程同步
            std::mutex _mtx;               // 互斥锁，保护条件变量
        };
    };
};


#endif //RUNLOOP_H
