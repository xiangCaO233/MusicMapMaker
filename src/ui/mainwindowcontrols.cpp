#include <audio/track/trackmanager.h>
#include <project/projectmanager.h>

#include "mainwindow.h"
#include "ui_mainwindow.h"

// 菜单项槽函数 - 控制显示和隐藏
void MainWindow::on_actionEditor_toggled(bool checked) {
    // 直接根据菜单的勾选状态来设置可见性
    editor->setVisible(checked);
    if (checked) {
        // 如果是显示，则将其带到最前
        trackmanager->activateWindow();
    }
}
void MainWindow::on_actionTrack_Manager_toggled(bool checked) {
    trackmanager->setVisible(checked);
    if (checked) {
        trackmanager->activateWindow();
    }
}

void MainWindow::on_actionProject_Manager_toggled(bool checked) {
    projectmanager->setVisible(checked);
    if (checked) {
        projectmanager->activateWindow();
    }
}
