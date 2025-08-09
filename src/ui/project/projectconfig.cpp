#include "projectconfig.h"

#include "ui_projectconfig.h"

ProjectConfig::ProjectConfig(QWidget *parent)
    : QWidget(parent), ui(new Ui::ProjectConfig) {
    ui->setupUi(this);
}

ProjectConfig::~ProjectConfig() { delete ui; }
