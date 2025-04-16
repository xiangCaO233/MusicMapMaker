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

void MFileBrowserController::on_open_file(QString& file_path) {}
void MFileBrowserController::on_rename_file(QString& file_path) {}
void MFileBrowserController::on_delete_file(QString& file_path) {}
void MFileBrowserController::on_show_properties(QString& path) {}
void MFileBrowserController::on_new_file_infolder(QString& file_path) {}
void MFileBrowserController::on_open_folder(QFileSystemModel* model,
                                            QModelIndex& index) {}
void MFileBrowserController::on_open_folder_as_project(QString& file_path) {
  // TODO(xiang 2025-04-16): 实现打开工程
  std::filesystem::path ppath(file_path.toStdString());
  XINFO("打开项目目录:[" + ppath.string() + "]");
  auto project = std::make_shared<MapWorkProject>(ppath);
  // 发送信号
  emit open_project(project);
}
