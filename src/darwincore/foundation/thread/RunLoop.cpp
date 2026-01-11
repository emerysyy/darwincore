//
// Created by Darwin Core on 2024/5/25.
//

#include <darwincore/foundation/thread/RunLoop.h>

using namespace darwincore::thread;

RunLoop::RunLoop() {

}

RunLoop::~RunLoop() {

}

void RunLoop::run() {
    std::unique_lock<std::mutex> lock(_mtx);
    _cv.wait(lock);
}

void RunLoop::stop() {
    std::unique_lock<std::mutex> lock(_mtx);
    _cv.notify_one();
}
