//
// Created by Darwin Core on 2023/9/11.
//

#ifndef MYLIB_TIMER_H
#define MYLIB_TIMER_H

#include <iostream>
#include <mutex>
#include <thread>
#include <map>
#include <functional>
#include <atomic>

namespace darwincore {
    namespace timer {

        // 定时器回调函数类型
        using TimerCallback = std::function<void()>;

        // ========================================
        // 定时器类
        // ========================================
        // 功能：
        // 1. 提供单次或重复执行的定时任务
        // 2. 支持动态启动和停止
        // 3. 由 TimerManager 统一管理
        // ========================================
        class Timer {
            friend class TimerManager;
        public:
            // 构造函数
            // id: 定时器唯一标识
            // interval: 执行间隔（毫秒）
            // repeat: 是否重复执行
            // callback: 回调函数
            Timer(int id, int interval, bool repeat, TimerCallback callback);
            ~Timer();

            // 启动定时器
            void start();

            // 停止定时器
            void stop();

            // 执行任务（内部调用）
            void doTask();

        private:
            TimerCallback _callback;  // 回调函数
            int _interval;            // 执行间隔（毫秒）
            bool _running;            // 运行标志
            bool _repeat;             // 是否重复执行
            int _id;                  // 定时器 ID
            int _remainTime;          // 剩余时间
        };


        // ========================================
        // 定时器管理器类（单例）
        // ========================================
        // 功能：
        // 1. 统一管理所有定时器
        // 2. 使用单例模式，确保全局唯一
        // 3. 支持定时器的创建、启动、停止和清理
        // ========================================
        class TimerManager {
        private:
            TimerManager();
            ~TimerManager();
            // 禁止拷贝和赋值
            TimerManager(const TimerManager&) = delete;
            TimerManager& operator=(const TimerManager&) = delete;

        public:
            // 获取单例实例
            static TimerManager &manager();

            // 创建并启动定时器
            // interval: 执行间隔（毫秒）
            // repeat: 是否重复执行
            // callback: 回调函数
            // 返回：定时器 ID
            int start(int interval, bool repeat, TimerCallback callback);

            // 停止指定的定时器
            void stop(int timerId);

            // 清理指定的定时器
            void clean(int timerId);

        private:
            // 定时器线程主函数
            void run();

        private:
            static std::atomic<int> currentId;       // 当前可用的 ID
            bool _running;                           // 运行标志
            std::mutex _timerMapMutex;               // 定时器映射的互斥锁
            std::map<int, Timer> _timerMap;          // 定时器映射（ID -> Timer）
            std::thread _timingThread;               // 定时器线程
        };

    };
};

#endif //MYLIB_TIMER_H
