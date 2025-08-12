#include <qdir.h>
#include <qlogging.h>
#include <ui_projectmanager.h>

#include <map/MapCanvas.hpp>
#include <util/mutil.hpp>

#include "projectmanager.h"

void ProjectManager::on_create_project_button_clicked() {
    // 使用文件夹选择器选择新项目创建的目标目录

    // 打开项目新建引
}

void ProjectManager::on_add_project_button_clicked() {
    // 使用文件夹选择器选择项目的目录
    auto dir = mutil::getDirectory(this, tr("select project directory"),
                                   QDir::homePath());

    if (!dir.isEmpty()) {
        open_project(dir.toStdString());
    } else {
        // 取消打开
        qDebug() << "取消打开项目";
    }

    // 打开项目
}

void ProjectManager::on_close_project_button_clicked() {
    // ui->project_list->selectionModel()->selection().
}
void ProjectManager::on_project_list_doubleClicked(const QModelIndex &index) {
    // 展示项目
    if (!ui->preference_button->isVisible()) {
        ui->preference_button->setVisible(true);
    }
    show_project(qobject_cast<QStandardItemModel *>(ui->project_list->model())
                     ->data(index, Qt::UserRole + 1)
                     .value<std::string>());
}

void ProjectManager::on_preference_button_clicked() {
    // 打开项目配置界面
    auto selection = ui->project_list->selectionModel();
    if (selection->hasSelection()) {
        auto index = selection->selectedIndexes()[0];
        auto project_name =
            qobject_cast<QStandardItemModel *>(ui->project_list->model())
                ->data(index, Qt::UserRole + 1)
                .value<std::string>();
        auto project = projects.find(project_name)->second.get();
        config_ui->bind_config(&project->project_config);
        if (!config_ui->isVisible()) {
            config_ui->show();
        }
    }
}

void ProjectManager::on_map_listView_doubleClicked(const QModelIndex &index) {
    // 打开谱面
    map_canvas->switch_map(
        qobject_cast<QStandardItemModel *>(ui->map_listView->model())
            ->data(index, Qt::UserRole + 1)
            .value<MMap *>());
}
