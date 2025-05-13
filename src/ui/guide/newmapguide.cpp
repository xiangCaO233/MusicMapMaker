#include "newmapguide.h"

#include "ui_newmapguide.h"

NewMapGuide::NewMapGuide(QWidget *parent)
    : QDialog(parent), ui(new Ui::NewMapGuide) {
    ui->setupUi(this);
}

NewMapGuide::~NewMapGuide() { delete ui; }
