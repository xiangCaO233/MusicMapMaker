#include "mmetas.h"

#include "ui_mmetas.h"

MMetas::MMetas(QWidget *parent) : QWidget(parent), ui(new Ui::MMetas) {
  ui->setupUi(this);
}

MMetas::~MMetas() { delete ui; }
