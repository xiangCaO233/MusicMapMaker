#include "minfo.h"

#include "ui_minfo.h"

MInfo::MInfo(QWidget *parent) : QWidget(parent), ui(new Ui::MInfo) {
  ui->setupUi(this);
  mmetas = ui->mapmeta_edit;
}

MInfo::~MInfo() { delete ui; }

// 使用主题
void MInfo::use_theme(GlobalTheme theme) { ui->mapmeta_edit->use_theme(theme); }
