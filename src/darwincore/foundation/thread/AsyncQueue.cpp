//
//  Created by Darwin Core on 2023/1/18.
//

#include <darwincore/foundation/thread/AsyncQueue.h>
#include <thread>
#include <unistd.h>

using namespace darwincore::thread;

int AsyncQueue::defaultQueueSize() {
    
    return (int)std::thread::hardware_concurrency(); // 硬件支持线程并行执行数量
    
//    return sysconf(_SC_NPROCESSORS_ONLN); // CPU核心数
}

AsyncQueue::AsyncQueue(int queueSize) : mQueueSize(queueSize), isRunning(true) {
    initial();
}

AsyncQueue::~AsyncQueue() {
    
    {
        std::unique_lock<std::mutex> lock(this->mQueueMutex);
        this->isRunning = false;
        this->mConditionVar.notify_one();
    }
    
    this->mConditionVar.notify_all();
    for (auto &th : mThreadArray) {
        th.join();
    }
    
    mThreadArray.clear();
}

void AsyncQueue::initial() {
    for (int i = 0; i < mQueueSize; i++) {
        
        
        mThreadArray.emplace_back([this, i](){
            
            std::string threadName = "emerys.asyncqueue." + std::to_string(i);
            pthread_setname_np(threadName.c_str());
            
            while (true) {
                
                std::function<void(void)> task;
                bool ignoreTask = false;
                do {
                    std::unique_lock<std::mutex> lock(this->mQueueMutex);
                    this->mConditionVar.wait(lock, [this](){
                        return !this->isRunning || !this->mTaskQueue.empty();
                    });
                    
                    if (this->mTaskQueue.empty()) {
                        ignoreTask = true;
                        break;
                    }
                    
                    task = std::move(this->mTaskQueue.front());
                    this->mTaskQueue.pop();
                } while(false);
                
                if (!ignoreTask) {
                    task();
                }
                if (task == nullptr) {
                    break;
                }
            }
            
        });
    }
}

AsyncQueue &AsyncQueue::queue() {
    static AsyncQueue queue(defaultQueueSize());
    return queue;
}
