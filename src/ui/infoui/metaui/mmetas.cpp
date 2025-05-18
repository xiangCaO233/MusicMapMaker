#include "mmetas.h"

#include <qmainwindow.h>

#include <memory>

#include "../../GlobalSettings.h"
#include "ui_mmetas.h"

MMetas::MMetas(QWidget *parent) : QWidget(parent), ui(new Ui::MMetas) {
    ui->setupUi(this);
    objinfo_ref = ui->objinfo_widget;
    timinginfo_ref = ui->timinginfo_widget;
}

MMetas::~MMetas() { delete ui; }

// 使用主题
void MMetas::use_theme(GlobalTheme theme) {
    objinfo_ref->use_theme(theme);
    timinginfo_ref->use_theme(theme);
}
