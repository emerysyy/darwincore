//
// Created by 李培 on 2024/10/20.
//

#include <darwincore/foundation/process/SharedMemory.h>
#include <sys/shm.h>
#include <sys/sysctl.h>
#include <sys/sem.h>
#include <xpc/xpc.h>

using namespace darwincore::process;

#define SHARE_MEM_SIZE_MAX (2*1024*1024)

int GetMaxSharedMemSize()
{
#if defined(__APPLE__)
    unsigned long maxShmSize = 0;
    size_t len = sizeof(maxShmSize);
    if(sysctlbyname("kern.sysv.shmmax", &maxShmSize, &len, NULL, 0) == 0)
    {
        unsigned long halfSize = maxShmSize / 2;
        return halfSize > SHARE_MEM_SIZE_MAX ? SHARE_MEM_SIZE_MAX : (int)halfSize;
    }
    else
    {
        return SHARE_MEM_SIZE_MAX;
    }
#else
    return SHARE_MEM_SIZE_MAX;
#endif
}

Semaphore::Semaphore(std::string path): _path(_path), _sem_id(0) {
}

Semaphore::~Semaphore() {
    uninitialize();
}

bool Semaphore::initialize(int offset, int value) {
    if (_path.empty()) {
        return false;
    }

    key_t key = ftok(_path.c_str(), offset);
    if (key == -1) {
        return false;
    }

    int semid = shmget(key, 1, 0666 | IPC_CREAT);
    if (semid == -1) {
        perror("Semaphore::initialize");
        return false;
    }
    _sem_id = semid;

    if (value > 0) {
        semctl(semid, 0, SETVAL, value);
    }
    return true;
}

void Semaphore::uninitialize() {
    if (_sem_id <= 0) {
        return;
    }
    semctl(_sem_id, 0, IPC_RMID);
    _sem_id = 0;
}

int Semaphore::wait(int timeout_ms) {
    if (_sem_id <= 0) {
        return -1;
    }

    struct sembuf sem;
    sem.sem_num = 0;
    sem.sem_op = -1;
    sem.sem_flg = SEM_UNDO | IPC_NOWAIT;

    int ret = 0;
    int err = 0;
    if (timeout_ms <= 0) {
        ret = semop(_sem_id, &sem, 1);
        err = errno;
    }
    else {
        int sleep_ms = 100;
        while (sleep_ms--) {
            ret = semop(_sem_id, &sem, 1);
            err = errno;
            if (ret == 0 || err == EAGAIN) {
                break;
            }

            usleep(sleep_ms);
            timeout_ms -= sleep_ms;
        }
    }

    if (ret == 0) {
        return 0;
    }
    else if (err == EIDRM) {
        return -2;
    }
    else if (err == EAGAIN) {
        return -3;
    }
    return -4;

}

int Semaphore::post() {
    if (_sem_id <= 0) {
        return -1;
    }

    struct sembuf sem;
    sem.sem_num = 0;
    sem.sem_op = 1;
    sem.sem_flg = SEM_UNDO;
    int ret = semop(_sem_id, &sem, 1);
    if (ret == 0) {
        return 0;
    }
    else if (errno == EIDRM) {
        return -2;
    }
    return -4;

}


/////

SharedMemory::SharedMemory(std::string path) : _path(path), _shm_id(0), _shm_ptr(nullptr), _shm_sem(path), _shm_mutex(path) {

}

SharedMemory::~SharedMemory() {

}

bool SharedMemory::open() {

    if (!getSharedMemory(true)) {
        return false;
    }

    if (!_shm_sem.initialize('B')) {
        return false;
    }

    if (!_shm_mutex.initialize('C', 1)) {
        return false;
    }

    return true;
}

bool SharedMemory::getSharedMemory(bool create) {
    if (_path.empty()) {
        return false;
    }

    if (_shm_id > 0) {
        return true;
    }

    key_t key = ftok(_path.c_str(), 'A');
    if (key == -1) {
        return false;
    }

    int flag = 0666;
    if (create) {
        flag = flag | IPC_CREAT;
    }

    _shm_id = shmget(key, GetMaxSharedMemSize(), flag);
    if (_shm_id < 0) {
        return false;
    }

    _shm_ptr = shmat(_shm_id, nullptr, 0);
    if (_shm_ptr == nullptr) {
        return false;
    }
}

bool SharedMemory::destroySharedMemory() {

    if (_shm_ptr != nullptr) {
        shmdt(_shm_ptr);
    }
    _shm_ptr = nullptr;

    if (_shm_id > 0) {
        shmctl(_shm_id, IPC_RMID, nullptr);
    }
    _shm_id = 0;
}
