//
// Created by Darwin Core on 2024/1/15.
//

#ifndef THROTTLE_H
#define THROTTLE_H

#include <iostream>
#include <functional>
#include <chrono>
#include <thread>
#include <map>
#include <mutex>

namespace darwincore {
    namespace throttle {
        // ========================================
        // 限流器类
        // ========================================
        // 功能：
        // 1. 提供事件限流机制，控制事件触发的频率
        // 2. 支持为不同的事件 ID 设置不同的时间间隔
        // 3. 当事件提交过于频繁时，会按指定的时间间隔进行节流
        // 用途：防止事件处理过于频繁，例如防抖、节流等场景
        // ========================================
        class Throttle {
        public:
            // 事件回调函数类型定义
            // event_id: 事件 ID
            using ThrottleEventCall = std::function<void(int event_id)>;

            Throttle();
            ~Throttle();

        public:
            // 注册一个事件及其时间间隔
            // event_id: 事件唯一标识符
            // interval_seconds: 事件触发的时间间隔（秒），即同一事件的最小间隔时间
            void registerEvent(int event_id, int interval_seconds);

            // 注册事件回调函数
            // call: 当事件需要触发时调用的回调函数
            void registerEventCall(ThrottleEventCall call);

            // 提交事件
            // event_id: 事件 ID，必须是已注册的事件
            // 说明：如果距离上次触发时间不足间隔时间，则不会触发回调
            void submit(int event_id);

        private:
            // 限流器工作线程主函数
            void run();

        private:
            // ========================================
            // 事件项结构（内部使用）
            // ========================================
            class EventItem {
            public:
                int event_id;              // 事件 ID
                int interval_seconds;      // 时间间隔（秒）
                long long next_call_time;   // 下次允许触发的时间戳（毫秒）
            };

            std::map<int, EventItem> event_map;      // 事件映射表（event_id -> EventItem）
            std::mutex mutex_event_map;              // 保护 event_map 的互斥锁

            std::thread work_thread;                 // 工作线程
            ThrottleEventCall event_call;            // 事件回调函数
            bool is_run;                             // 运行标志
        };
    };
};


#endif //THROTTLE_H
