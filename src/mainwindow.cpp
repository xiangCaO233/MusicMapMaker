#include "mainwindow.h"

#include "./ui_mainwindow.h"
#include "mmm/map/osu/OsuMap.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  // 获得画布指针
  canvas = ui->canvas;

  // 设置map
  auto omap = std::make_shared<OsuMap>();
  omap->load_from_file(
      "../resources/map/Lia - Poetry of Birds/Lia - Poetry of Birds "
      "(xiang_233) [full version].osu");
  canvas->switch_map(omap);
}

MainWindow::~MainWindow() { delete ui; }
