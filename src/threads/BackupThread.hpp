#ifndef M_BACKUPTHREAD_H
#define M_BACKUPTHREAD_H

#include <thread>
class BackupThread {
    std::thread t;

   public:
    // 构造BackupThread
    BackupThread();
    // 析构BackupThread
    virtual ~BackupThread();
};

#endif  // M_BACKUPTHREAD_H
