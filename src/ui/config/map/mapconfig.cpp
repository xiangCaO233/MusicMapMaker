#include "mapconfig.h"

#include "ui_mapconfig.h"

MapConfig::MapConfig(QWidget *parent) : QWidget(parent), ui(new Ui::MapConfig) {
    ui->setupUi(this);
}

MapConfig::~MapConfig() { delete ui; }
