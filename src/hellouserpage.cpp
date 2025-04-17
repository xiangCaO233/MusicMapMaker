#include "hellouserpage.h"

#include "ui_hellouserpage.h"

HelloUserPage::HelloUserPage(QWidget *parent)
    : QWidget(parent), ui(new Ui::HelloUserPage) {
  ui->setupUi(this);
}

HelloUserPage::~HelloUserPage() { delete ui; }
