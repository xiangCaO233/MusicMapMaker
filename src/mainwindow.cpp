#include "mainwindow.h"

#include "./ui_mainwindow.h"
#include "mmm/map/osu/OsuMap.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  // 获得画布指针
  canvas = ui->canvas;

  // 设置map
  auto omap6k1 = std::make_shared<OsuMap>();
  omap6k1->load_from_file(
      "../resources/map/Designant - Designant/Designant - Designant. (Benson_) "
      "[Designant].osu");
  auto omap4k1 = std::make_shared<OsuMap>();
  omap4k1->load_from_file(
      "../resources/map/Lia - Poetry of Birds/Lia - Poetry of Birds "
      "(xiang_233) [full version].osu");
  auto omap4k2 = std::make_shared<OsuMap>();
  omap4k2->load_from_file(
      "../resources/map/Haruka Kiritani  Shizuku Hino Mori  Hatsune Miku - "
      "shojo rei/Haruka Kiritani  Shizuku Hino Mori  Hatsune Miku - shojo rei "
      "(xiang_233) [(LN)NM lv.29].osu");
  canvas->switch_map(omap6k1);
}

MainWindow::~MainWindow() { delete ui; }
