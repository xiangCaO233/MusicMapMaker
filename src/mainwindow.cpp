#include "mainwindow.h"

#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  // 获得画布引用
  auto &canvas = ui->canvas;
  auto rect = QRectF(100, 100, 50, 50);
  canvas->renderer_manager->addRect(rect, nullptr, Qt::red, false);
  canvas->renderer_manager->renderAll();
}

MainWindow::~MainWindow() { delete ui; }
