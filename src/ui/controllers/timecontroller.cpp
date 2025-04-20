#include "timecontroller.h"

#include "ui_timecontroller.h"

TimeController::TimeController(QWidget *parent)
    : QWidget(parent), ui(new Ui::TimeController) {
  ui->setupUi(this);
}

TimeController::~TimeController() { delete ui; }
