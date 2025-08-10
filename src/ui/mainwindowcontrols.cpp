#include <audio/track/trackmanager.h>
#include <project/projectmanager.h>
#include <ui_mainwindow.h>

#include "mainwindow.h"

// 菜单项槽函数 - 控制显示和隐藏
void MainWindow::on_actionTrack_Manager_toggled(bool checked) {
    ui->track_manager_dock->setVisible(checked);
    // if (checked) {
    //     trackmanager->activateWindow();
    // }
}

void MainWindow::on_actionProject_Manager_toggled(bool checked) {
    ui->project_dock->setVisible(checked);
    // if (checked) {
    //     projectmanager->activateWindow();
    // }
}
