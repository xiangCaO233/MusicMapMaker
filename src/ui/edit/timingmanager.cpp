#include "timingmanager.h"

#include "ui_timingmanager.h"

TimingManager::TimingManager(QWidget *parent)
    : QWidget(parent), ui(new Ui::TimingEditor) {
    ui->setupUi(this);
}

TimingManager::~TimingManager() { delete ui; }
