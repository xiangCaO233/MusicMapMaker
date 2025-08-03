#include "mainwindow.h"

#include <audio/track/trackmanager.h>
#include <project/projectmanager.h>

#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    editor = new MapEditor();
    projectmanager = new ProjectManager();
    trackmanager = new TrackManager();

    editor->hide();
    projectmanager->hide();
    trackmanager->hide();

    // 捕获ui指针
    auto capui = ui;

    connect(editor, &MapEditor::close_signal,
            [capui]() { capui->actionEditor->setChecked(false); });
    connect(trackmanager, &TrackManager::close_signal,
            [capui]() { capui->actionTrack_Manager->setChecked(false); });
    connect(projectmanager, &ProjectManager::close_signal,
            [capui]() { capui->actionProject_Manager->setChecked(false); });
}

MainWindow::~MainWindow() {
    // editor->hide();
    // projectmanager->hide();
    // trackmanager->hide();

    delete editor;
    delete trackmanager;
    delete projectmanager;
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent* e) {
    editor->close();
    trackmanager->close();
    projectmanager->close();
}
