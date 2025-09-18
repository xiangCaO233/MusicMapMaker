#include <mainwindow.h>
#include <projectmanager.h>
#include <qdir.h>
#include <qlogging.h>
#include <ui_projectmanager.h>

#include <map/MapCanvas.hpp>
#include <util/mutil.hpp>

// void ProjectManager::on_create_project_button_clicked() {
//     // 使用文件夹选择器选择新项目创建的目标目录
//
//     // 打开项目新建引导
// }
//
// void ProjectManager::on_add_project_button_clicked() {
//     // 使用文件夹选择器选择项目的目录
//     auto dir = mutil::getDirectory(this, tr("select project directory"),
//                                    QDir::homePath());
//     if (!dir.isEmpty()) {
//         auto ppath = dir.toStdString();
//         // 打开项目
//         emit openProject(ppath);
//     } else {
//         // 取消打开
//         qDebug() << "取消打开项目";
//     }
// }

// void ProjectManager::on_close_project_button_clicked() {
//     auto selection = ui->project_list->selectionModel();
//     if (selection->hasSelection()) {
//         auto index = selection->selectedIndexes()[0];
//         auto project_name =
//             qobject_cast<QStandardItemModel *>(ui->project_list->model())
//                 ->data(index, Qt::UserRole + 1)
//                 .value<std::string>();
//         emit closeProject(project_name);
//     }
// }
//
// void ProjectManager::on_project_list_doubleClicked(const QModelIndex &index)
// {
//     // 切换展示项目
//     if (!ui->preference_button->isVisible()) {
//         ui->preference_button->setVisible(true);
//     }
//     auto clicked_project_name =
//         qobject_cast<QStandardItemModel *>(ui->project_list->model())
//             ->data(index, Qt::UserRole + 1)
//             .value<std::string>();
//     service->selectProject(clicked_project_name);
// }
//
// void ProjectManager::on_preference_button_clicked() {
//     // 打开项目配置界面
//     auto selection = ui->project_list->selectionModel();
//     if (selection->hasSelection()) {
//         if (!config_ui->isVisible()) {
//             config_ui->setStyleSheet(MainWindow::global_style_sheet);
//             config_ui->show();
//         }
//     }
// }

void ProjectManager::on_map_listView_doubleClicked(const QModelIndex &mindex) {
    // 打开谱面

    // 获取当前激活的项目
    auto project = service->currentPorject();
    auto map = qobject_cast<QStandardItemModel *>(ui->map_listView->model())
                   ->data(mindex, Qt::UserRole + 1)
                   .value<MMap *>();
    service->selectMap(project->project_path.filename().generic_string(), map);
}
