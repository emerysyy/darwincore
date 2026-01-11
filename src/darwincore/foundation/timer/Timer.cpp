//
// Created by Darwin Core on 2023/9/11.
//

#include <darwincore/foundation/timer/Timer.h>
#include <utility>
#include <darwincore/foundation/common/Common.h>
#include <darwincore/foundation/thread/AsyncQueue.h>

using namespace darwincore::timer;
using namespace darwincore::common;
using namespace darwincore::thread;

std::atomic<int> TimerManager::currentId(0);

TimerManager::TimerManager() : _running(true), _timingThread(&TimerManager::run, this) {
}

TimerManager::~TimerManager() {
    _running = false;
    if (_timingThread.joinable()) {
        _timingThread.join();
    }
}

Timer::Timer(int id, int interval, bool repeat, TimerCallback callback) : _id(id),
                                                                          _interval(interval),
                                                                          _repeat(repeat),
                                                                          _callback(std::move(callback)),
                                                                          _running(false),
                                                                          _remainTime(interval * 1000) {
    start();
}

Timer::~Timer() {
    stop();
}

void Timer::start() {
    if (_running) {
        return;
    }
    _running = true;
}

void Timer::stop() {
    if (!_running) {
        return;
    }
    _callback = nullptr;
}

void Timer::doTask() {
    _remainTime = -1;
    if (_callback) {
        auto &callback = _callback;
        AsyncQueue::queue().submit([&]() {
            callback();
            if (!this->_repeat) {
                this->stop();
            } else {
                this->_remainTime = this->_interval * 1000;
            }
        });
    }
}

//****************//
//  TimerManager  //
//****************//

TimerManager &TimerManager::manager() {
    static TimerManager defaultManager;
    return defaultManager;
}

int TimerManager::start(int interval, bool repeat, TimerCallback callback) {

    int id = ++currentId;
    Timer timer(id, interval, repeat, std::move(callback));

    {
        std::unique_lock<std::mutex> lock(_timerMapMutex);
        _timerMap.insert(std::make_pair(id, std::move(timer)));
    }

    return id;
}

void TimerManager::stop(int timerId) {
    std::unique_lock<std::mutex> lock(_timerMapMutex);
    clean(timerId);
}

void TimerManager::clean(int timerId) {
    if (_timerMap.count(timerId)) {
        Timer &timer = _timerMap.at(timerId);
        timer.stop();
        _timerMap.erase(timerId);
    }
}

void TimerManager::run() {
    while (_running) {
        {
            std::unique_lock<std::mutex> lock(_timerMapMutex);
            for (auto it = _timerMap.begin(); it != _timerMap.end();) {
                Timer &timer = it->second;
                if (!timer._running) {
                    it = _timerMap.erase(it);
                    continue;
                }

                if (timer._remainTime == 0) {
                    timer.doTask();
                } else if (timer._remainTime > 0) {
                    timer._remainTime--;
                }
                it++;
            }
        }
        sleep_ms(1);
    }

    // stop
    std::unique_lock<std::mutex> lock(_timerMapMutex);
    for (auto &it: _timerMap) {
        Timer &timer = it.second;
        clean(timer._id);
    }
}