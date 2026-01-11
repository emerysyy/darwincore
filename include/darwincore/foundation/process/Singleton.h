//
// Created by 李培 on 2026/1/7.
//

#ifndef SINGLETON_H
#define SINGLETON_H

namespace darwincore {
    namespace process {

        // ========================================
        // 单例模式模板类
        // ========================================
        // 功能：
        // 1. 提供线程安全的单例实现（基于静态局部变量）
        // 2. 使用 C++11 保证静态局部变量的线程安全初始化
        // 3. 禁止拷贝和移动，确保单例唯一性
        // ========================================
        template<class T>
        class Singleton {
        public:
            // 获取单例实例（线程安全）
            static T* getInstance() {
                static T instance;
                return &instance;
            }

        protected:
            // 构造函数和析构函数设为 protected，允许继承
            Singleton() {};
            virtual ~Singleton() = default;

        public:
            // 禁止拷贝构造和赋值
            Singleton(const Singleton&) = delete;
            Singleton& operator=(const Singleton&) = delete;
            // 禁止移动构造和赋值
            Singleton(Singleton&&) = delete;
            Singleton& operator=(Singleton&&) = delete;

        };
    };
 };





#endif //SINGLETON_H
