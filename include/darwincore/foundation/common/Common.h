//
// Created by Darwin Core on 2023/1/17.
//

#ifndef MYLIB_COMMON_H
#define MYLIB_COMMON_H

#include <iostream>

namespace darwincore {
    namespace common {
        // ========================================
        // 通用工具函数
        // ========================================
        // 功能：
        // 提供常用的通用工具函数，如线程休眠等
        // ========================================

        // 线程休眠（毫秒）
        // milliseconds: 休眠的毫秒数
        void sleep_ms(int milliseconds);

        // 线程休眠（秒）
        // seconds: 休眠的秒数
        void sleep_s(int seconds);

        // 通用工具类（预留扩展）
        class Common {

        };
    };
};





#endif //MYLIB_COMMON_H
