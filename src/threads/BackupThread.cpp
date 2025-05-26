#include "BackupThread.h"

#include <chrono>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <mutex>
#include <string>
#include <system_error>

#include "../canvas/map/MapWorkspaceCanvas.h"
#include "../mmm/MapWorkProject.h"
#include "../util/mutil.h"
#include "GlobalSettings.h"
#include "colorful-log.h"
#include "mainwindow.h"

// 构造BackupThread
BackupThread::BackupThread(MapWorkspaceCanvas* canvas) : canvas_contex(canvas) {
    thread = std::thread(&BackupThread::run, this);
    thread.detach();
}

// 析构BackupThread
BackupThread::~BackupThread() {
    exit = true;
    threadcv.notify_all();
}

// 修改暂停状态
void BackupThread::setPaused(bool flag) { pause_bkup.store(flag); }

// 备份线程线程函数
void BackupThread::run() {
    while (!exit) {
        std::unique_lock<std::mutex> lock(threadmtx);
        threadcv.wait_for(lock, std::chrono::milliseconds(30000),
                          [&]() { return exit.load(); });
        if (!pause_bkup) {
            backup_map_thread();
        }
        if (!pause_autosave) {
            auto_save_thread();
        }
    }
}

// 写出谱面文件函数
bool BackupThread::try_write_map2file(std::string_view rfile) {
    // 只需写出文件
    try {
        canvas_contex->working_map->write_to_file(rfile.data());
        XINFO("保存文件[" + std::string(rfile) + "]成功:");
        return true;
    } catch (std::exception e) {
        XERROR("保存文件[" + std::string(rfile) + "]失败:" + e.what());
        return false;
    }
}

// 自动保存线程函数
void BackupThread::auto_save_thread() {
    if (canvas_contex && canvas_contex->working_map) {
        // 保存为mmm

        // 缓存文件
        auto temp_filename = mutil::sanitizeFilename(
            canvas_contex->working_map->title_unicode + "-" +
            std::to_string(canvas_contex->working_map->orbits) + "k-" +
            canvas_contex->working_map->version + "-temp.mmm");

        // 实际的自动保存的目标文件
        auto des_autosave_filename = mutil::sanitizeFilename(
            canvas_contex->working_map->title_unicode + "-" +
            std::to_string(canvas_contex->working_map->orbits) + "k-" +
            canvas_contex->working_map->version + ".mmm");

        auto temp_file_str =
            (canvas_contex->working_map->project_reference->ppath /
             temp_filename)
                .generic_string();
        auto des_autosave_file_str =
            (canvas_contex->working_map->project_reference->ppath /
             des_autosave_filename)
                .generic_string();

        if (try_write_map2file(temp_file_str)) {
            if (std::filesystem::exists(des_autosave_file_str)) {
                std::error_code ec;
                std::filesystem::remove(des_autosave_file_str);
                std::filesystem::rename(temp_file_str, des_autosave_file_str,
                                        ec);
                if (ec) {
                    XWARN("替换失败:" + std::to_string(ec.value()));
                } else {
                    XINFO("已自动保存");
                }
            }
        }
    }
}

std::string get_current_time() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm local_time = *std::localtime(&now_time);

    std::ostringstream oss;
    oss << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

// 备份谱面线程函数
void BackupThread::backup_map_thread() {
    if (canvas_contex && canvas_contex->working_map) {
        auto bkup_file_path =
            canvas_contex->working_map->project_reference->ppath /
            MainWindow::settings.backup_relative_path;
        while (canvas_contex->working_map->map_backup_paths_queue.size() >
               MainWindow::settings.backup_map_file_count) {
            // 移除队首备份
            auto autoremove =
                canvas_contex->working_map->map_backup_paths_queue.front();
            try {
                std::filesystem::remove(autoremove);
                XERROR("已移除过时备份文件[" + autoremove.generic_string() +
                       "]");
            } catch (std::exception e) {
                XERROR("移除备份文件[" + autoremove.generic_string() +
                       "]失败:" + e.what());
            }
            canvas_contex->working_map->map_backup_paths_queue.pop_front();
        }

        // 保存为mmm
        auto bak_filename = mutil::sanitizeFilename(
            "(" + get_current_time() + ")-" +
            canvas_contex->working_map->title_unicode + "-" +
            std::to_string(canvas_contex->working_map->orbits) + "k-" +
            canvas_contex->working_map->version + ".mmm.bak");

        auto bak_filestr = (bkup_file_path / bak_filename).generic_string();
        if (try_write_map2file(bak_filestr)) {
            canvas_contex->working_map->map_backup_paths_queue.push_back(
                bak_filestr);
            XINFO("已自动备份");
        }
    }
}
