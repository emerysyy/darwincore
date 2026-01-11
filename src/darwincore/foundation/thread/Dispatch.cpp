//
// Created by Darwin Core on 2023/1/17.
//

#include <darwincore/foundation/thread/Dispatch.h>

#include <utility>

using namespace darwincore::thread;

Runnable::Runnable(Runnable::Callback callback) : _running(true),  _call(std::move(callback)), _t(&Runnable::run, this) {

}

Runnable::~Runnable() {
    _running = false;
}

void Runnable::run() {
    if (_running && _call) {
        _call();
    }
}
