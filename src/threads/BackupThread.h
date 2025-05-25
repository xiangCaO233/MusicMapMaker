#ifndef M_BACKUPTHREAD_H
#define M_BACKUPTHREAD_H

#include <condition_variable>
#include <thread>

class BackupThread {
    // 退出线程
    std::atomic<bool> exit{false};

    std::thread thread;
    std::condition_variable threadcv;
    std::mutex threadmtx;

   public:
    // 构造BackupThread
    BackupThread();
    // 析构BackupThread
    virtual ~BackupThread();
};

#endif  // M_BACKUPTHREAD_H
