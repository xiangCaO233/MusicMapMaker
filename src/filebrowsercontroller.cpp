#include "filebrowsercontroller.h"

#include <qtmetamacros.h>

#include <filesystem>

#include "log/colorful-log.h"
#include "mmm/MapWorkProject.h"

// 构造MFileBrowserController
MFileBrowserController::MFileBrowserController() {}
// 析构MFileBrowserController
MFileBrowserController::~MFileBrowserController() = default;

// TODO(xiang 2025-04-16): 实现文件管理器功能

// 打开文件
void MFileBrowserController::on_menu_open_file(QString& file_path) {}

// 重命名文件
void MFileBrowserController::on_menu_rename_file(QString& file_path) {}

// 删除文件
void MFileBrowserController::on_menu_delete_file(QString& file_path) {}

// 显示属性
void MFileBrowserController::on_menu_show_properties(QString& path) {}

// 在文件夹内新建文件
void MFileBrowserController::on_menu_new_file_infolder(QString& file_path) {}

// 打开文件夹
void MFileBrowserController::on_menu_open_folder(QFileSystemModel* model,
                                                 QModelIndex& index) {}

// 将文件夹打开为项目
void MFileBrowserController::on_open_folder_as_project(QString& file_path) {
  // TODO(xiang 2025-04-16): 实现打开工程
  std::filesystem::path ppath(file_path.toStdString());
  XINFO("打开项目目录:[" + ppath.string() + "]");
  auto project = std::make_shared<MapWorkProject>(ppath);
  // 发送信号
  emit open_project(project);
}

// 文件管理器
// 地址栏确认事件
void MFileBrowserController::on_address_confirm_clicked() {
  // 跳转目录
}

void MFileBrowserController::on_file_browser_treeview_doubleClicked(
    const QModelIndex& index) {}

void MFileBrowserController::on_cdup_clicked() {}

void MFileBrowserController::on_cdnext_clicked() {}

void MFileBrowserController::on_search_clicked() {}
