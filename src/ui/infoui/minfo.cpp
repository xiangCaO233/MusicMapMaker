#include "minfo.h"

#include "ui_minfo.h"

MInfo::MInfo(QWidget *parent) : QWidget(parent), ui(new Ui::MInfo) {
  ui->setupUi(this);
}

MInfo::~MInfo() { delete ui; }
