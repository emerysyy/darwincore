//
// Created by Darwin Core on 2023/1/17.
//

#include <darwincore/foundation/common/Common.h>
#include <thread>
#include <unistd.h>
#include <chrono>

void darwincore::common::sleep_ms(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

void darwincore::common::sleep_s(int seconds) {
    sleep(seconds);
}
