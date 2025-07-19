#include "mainwindow.h"

#include "canvas/GLCanvas.hpp"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    auto canvas = ui->canvas_container->canvas.data();
    connect(canvas, &GLCanvas::update_window_suffix, this,
            &MainWindow::update_title_suffix);
}

// 更新标题后缀
void MainWindow::update_title_suffix(const QString &suffix) {
    setWindowTitle("MusicMapMaker-->" + suffix);
}

MainWindow::~MainWindow() { delete ui; }
