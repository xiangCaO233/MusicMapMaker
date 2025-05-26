#ifndef M_BACKUPTHREAD_H
#define M_BACKUPTHREAD_H

#include <condition_variable>
#include <deque>
#include <filesystem>
#include <string_view>
#include <thread>

class MapWorkspaceCanvas;

class BackupThread {
    // 退出线程
    std::atomic<bool> exit{false};
    std::atomic<bool> pause_bkup{false};
    std::atomic<bool> pause_autosave{false};

    std::thread thread;
    std::condition_variable threadcv;
    std::mutex threadmtx;

    MapWorkspaceCanvas* canvas_contex;

    // 备份线程线程函数
    void run();

    // 备份函数
    bool try_write_map2file(std::string_view rfile);

    // 自动保存线程函数
    void auto_save_thread();

    // 备份谱面线程函数
    void backup_map_thread();

   public:
    // 构造BackupThread
    BackupThread(MapWorkspaceCanvas* canvas);

    // 析构BackupThread
    virtual ~BackupThread();

    // 修改暂停状态
    void setPaused(bool flag);
};

#endif  // M_BACKUPTHREAD_H
