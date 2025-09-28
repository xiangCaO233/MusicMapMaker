#include <config/project/projectconfig.h>
#include <ui_projectconfig.h>

#include <mmm/project/MProject.hpp>

ProjectConfig::ProjectConfig(QWidget *parent)
    : QWidget(parent), ui(new Ui::ProjectConfig) {
    ui->setupUi(this);
    hideConfigs();
}

ProjectConfig::~ProjectConfig() { delete ui; }

// 隐藏配置组件
void ProjectConfig::hideConfigs() { ui->scrollAreaWidgetContents->hide(); }

// 显示配置组件
void ProjectConfig::showConfigs() const {
    ui->scrollAreaWidgetContents->show();
}

// 绑定配置
void ProjectConfig::bind_config(MProject *project) {
    config = project->cfg();
    QSignalBlocker nameBlocker(ui->project_name_edit);
    QSignalBlocker topPosBlocker(ui->toppos_spinner);
    QSignalBlocker rightPosBlocker(ui->rightpos_spinner);
    QSignalBlocker bottomPosBlocker(ui->bottompos_spinner);
    QSignalBlocker leftPosBlocker(ui->leftpos_spinner);
    QSignalBlocker widthScaleBlocker(ui->object_width_scale_slider);
    QSignalBlocker heightScaleBlocker(ui->object_height_scale_slider);
    QSignalBlocker judgelinePosBlocker(ui->judgeline_pos_slider);

    // 更新项目配置控件
    ui->project_name_edit->setText(
        QString::fromStdString(config->project_name));
    ui->toppos_spinner->setValue(config->canvas_config.canvas_layout.x);
    ui->rightpos_spinner->setValue(config->canvas_config.canvas_layout.y);
    ui->bottompos_spinner->setValue(config->canvas_config.canvas_layout.z);
    ui->leftpos_spinner->setValue(config->canvas_config.canvas_layout.w);
    ui->object_width_scale_slider->setValue(
        config->canvas_config.object_width_scale * 100.0);
    ui->object_height_scale_slider->setValue(
        config->canvas_config.object_height_scale * 100.0);
    ui->judgeline_pos_slider->setValue(config->canvas_config.judgeline_pos *
                                       100.0);
    emit layout_changed();

    showConfigs();
}
