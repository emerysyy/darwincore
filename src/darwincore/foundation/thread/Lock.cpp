//
// Created by Darwin Core on 2024/3/27.
//

#include <darwincore/foundation/thread/Lock.h>

using namespace darwincore::thread;

ReadWriteLock::ReadWriteLock() : _readers(0), _writers(0), _waitWriters(0) {

}

ReadWriteLock::~ReadWriteLock() {

}

void ReadWriteLock::lockWrite() {
    std::unique_lock<std::mutex> lock(this->_mutex);
    this->_waitWriters++;
    while(this->_readers > 0 || this->_writers > 0) {
        _cv_writer.wait(lock);
    }
    this->_writers++;
    this->_waitWriters--;
}

void ReadWriteLock::unlockWrite() {
    std::unique_lock<std::mutex> lock(this->_mutex);
    if (this->_writers == 0) {
        return;
    }
    this->_writers--;
    if (this->_writers == 0 && this->_waitWriters == 0) {
        _cv_reader.notify_all();
    }
    else {
        _cv_writer.notify_one();
    }
}

void ReadWriteLock::lockRead() {
    std::unique_lock<std::mutex> lock(this->_mutex);
    while(this->_waitWriters > 0 || this->_writers > 0) {
        _cv_reader.wait(lock);
    }
    this->_readers++;
}

void ReadWriteLock::unlockRead() {
    std::unique_lock<std::mutex> lock(this->_mutex);
    if (this->_readers == 0) {
        return;
    }
    this->_readers--;
    if (this->_readers == 0) {
        this->_cv_writer.notify_one();
    }
}

//************************
// Semaphore
//************************

Semaphore::Semaphore(int count): _count(count) {

}

Semaphore::~Semaphore() {

}

void Semaphore::post() {
    std::unique_lock<std::mutex> lock(_mutex);
    _count++;
    _cv.notify_one();
}

bool Semaphore::wait(int for_ms) {
    std::unique_lock<std::mutex> lock(_mutex);
    bool ready = true;
    if (for_ms > 0) {
        ready = _cv.wait_for(lock, std::chrono::milliseconds(for_ms), [&]{return _count > 0;});
    }
    else {
        _cv.wait(lock, [&]{return _count > 0;});
    }
    if (ready) {
        _count--;
    }

    return ready;
}
