#include "mainwindow.h"

#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  // 获得画布指针
  canvas = ui->canvas;
}

MainWindow::~MainWindow() { delete ui; }
