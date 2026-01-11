//
// Created by Darwin Core on 2024/1/15.
//

#include <darwincore/foundation/throttle/Throttle.h>

using namespace darwincore::throttle;

Throttle::Throttle() : is_run(true), work_thread(&Throttle::run, this) {
}

Throttle::~Throttle() {
    is_run = false;
    if (work_thread.joinable()) {
        work_thread.join();
    }
}

void Throttle::registerEvent(int event_id, int interval_seconds) {
    std::unique_lock<std::mutex> lock(mutex_event_map);
    event_map[event_id] = {event_id, interval_seconds, 0};
}

void Throttle::registerEventCall(ThrottleEventCall call) {
    event_call = std::move(call);
}

void Throttle::submit(int event_id) {
    long long currentTime = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    std::unique_lock<std::mutex> lock(mutex_event_map);
    auto it = event_map.find(event_id);
    if (it != event_map.end()) {
        if (it->second.next_call_time == 0) { // 没有此类事件
            it->second.next_call_time = currentTime + it->second.interval_seconds;
        }
        else { // 已有此类事件，等待事件执行
        }
    }
}

void Throttle::run() {
    while (is_run) {
        long long currentTime = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        std::unique_lock<std::mutex> lock(mutex_event_map);
        for (auto& it : event_map) {

            bool hasCall = event_call != nullptr;
            bool hasEvent = it.second.next_call_time > 0;
            bool isReay = currentTime >= it.second.next_call_time;

            if (hasCall && hasEvent && isReay) {
                it.second.next_call_time = 0;
                event_call(it.first);
            }
        }
        lock.unlock();
        std::this_thread::sleep_for(std::chrono::seconds(1)); // 每秒检查一次
    }
}
