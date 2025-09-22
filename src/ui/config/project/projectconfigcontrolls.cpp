#include <config/project/projectconfig.h>
#include <ui_projectconfig.h>

#include <mmm/project/MProjectConfig.hpp>

void ProjectConfig::on_toppos_spinner_valueChanged(double arg1) {
    config->canvas_config.canvas_layout.x = arg1;
    emit layout_changed();
}

void ProjectConfig::on_rightpos_spinner_valueChanged(double arg1) {
    config->canvas_config.canvas_layout.y = arg1;
    emit layout_changed();
}

void ProjectConfig::on_bottompos_spinner_valueChanged(double arg1) {
    config->canvas_config.canvas_layout.z = arg1;
    emit layout_changed();
}

void ProjectConfig::on_leftpos_spinner_valueChanged(double arg1) {
    config->canvas_config.canvas_layout.w = arg1;
    emit layout_changed();
}

void ProjectConfig::on_object_width_scale_slider_valueChanged(int value) {
    config->canvas_config.object_width_scale = double(value) / 100.0;
    ui->object_width_scale_value_label->setText(QString("%1%%").arg(value));
}

void ProjectConfig::on_object_height_scale_slider_valueChanged(int value) {
    config->canvas_config.object_height_scale = double(value) / 100.0;
    ui->object_height_scale_value_label->setText(QString("%1%%").arg(value));
}
