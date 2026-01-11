//
// Created by Darwin Core on 2023/1/17.
//

#ifndef MYLIB_DISPATCH_H
#define MYLIB_DISPATCH_H

#include <thread>
#include <functional>

namespace darwincore {
    namespace thread {

        // ========================================
        // 任务分发类
        // ========================================
        // 功能：
        // 1. 提供异步和同步执行任务的方法
        // 2. 异步执行：在新线程中执行任务，不阻塞调用线程
        // 3. 同步执行：在新线程中执行任务，阻塞调用线程直到任务完成
        // ========================================
        class Dispatch {
        public:
            // 异步执行任务（不阻塞）
            // block: 可调用对象（函数、lambda 表达式等）
            template<typename Block>
            static void async(Block block);

            // 同步执行任务（阻塞）
            // block: 可调用对象（函数、lambda 表达式等）
            template<typename Block>
            static void sync(Block block);
        };

        // ========================================
        // 可运行对象类
        // ========================================
        // 功能：
        // 封装一个可在独立线程中运行的任务
        // ========================================
        class Runnable {
            // 回调函数类型定义
            using Callback = std::function<void()>;

        public:
            // 构造函数
            // callback: 要执行的任务回调函数
            explicit Runnable(Callback callback);
            ~Runnable();

        private:
            // 任务执行函数（内部调用）
            void run();

        private:
            bool _running;       // 运行标志
            Callback _call;     // 回调函数
            std::thread _t;      // 执行线程
        };
    };
};

// 模板的定义需要在头文件
template<typename Block>
void darwincore::thread::Dispatch::async(Block block) {
    std::thread th(block);
    th.detach();  // 分离线程，使其在后台独立运行
}

template<typename Block>
void darwincore::thread::Dispatch::sync(Block block) {
    std::thread th(block);
    th.join();  // 等待线程完成
}



#endif

//MYLIB_DISPATCH_H
