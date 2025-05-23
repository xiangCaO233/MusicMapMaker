#include "mmetas.h"

#include <qmainwindow.h>

#include <memory>

#include "../../GlobalSettings.h"
#include "ui_mmetas.h"

MMetas::MMetas(QWidget *parent) : QWidget(parent), ui(new Ui::MMetas) {
    ui->setupUi(this);
    objinfo_ref = ui->objinfo_widget;
    timinginfo_ref = ui->timinginfo_widget;
    time_audio_controller_ref = ui->time_controller;
}

MMetas::~MMetas() { delete ui; }

void MMetas::switch_map(std::shared_ptr<MMap> map) {
    binding_map = map;
    if (binding_map) {
        // 更新谱面元数据
    }
}

// 使用主题
void MMetas::use_theme(GlobalTheme theme) {
    objinfo_ref->use_theme(theme);
    timinginfo_ref->use_theme(theme);

    // 设置时间控制器主题
    ui->time_controller->use_theme(theme);
}
