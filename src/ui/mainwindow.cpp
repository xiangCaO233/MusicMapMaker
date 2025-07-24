#include "mainwindow.h"

#include <audio/trackmanager.h>
#include <project/projectmanager.h>

#include "canvas/GLCanvas.hpp"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    auto canvas = ui->canvas_container->canvas.data();
    connect(canvas, &GLCanvas::update_window_suffix, this,
            &MainWindow::update_title_suffix);

    projectmanager = new ProjectManager();
    trackmanager = new TrackManager();

    projectmanager->hide();
    trackmanager->hide();

    connect(trackmanager, &TrackManager::close_signal, this,
            &MainWindow::trackmanager_close_slot);
    connect(projectmanager, &ProjectManager::close_signal, this,
            &MainWindow::projectmanager_close_slot);
}

// 更新标题后缀
void MainWindow::update_title_suffix(const QString& suffix) {
    setWindowTitle("MusicMapMaker-->" + suffix);
}

MainWindow::~MainWindow() {
    delete trackmanager;
    delete projectmanager;
    delete ui;
}

// 菜单项槽函数 - 控制显示和隐藏
void MainWindow::on_actionTrack_Manager_toggled(bool checked) {
    // 直接根据菜单的勾选状态来设置可见性
    trackmanager->setVisible(checked);
    if (checked) {
        // 如果是显示，则将其带到最前
        trackmanager->activateWindow();
    }
}

void MainWindow::on_actionProject_Manager_toggled(bool checked) {
    projectmanager->setVisible(checked);
    if (checked) {
        projectmanager->activateWindow();
    }
}

// 响应管理器关闭事件
void MainWindow::trackmanager_close_slot(
    [[maybe_unused]] HideableToolWindow* wptr) {
    ui->actionTrack_Manager->setChecked(false);
}
void MainWindow::projectmanager_close_slot(
    [[maybe_unused]] HideableToolWindow* wptr) {
    ui->actionProject_Manager->setChecked(false);
}
