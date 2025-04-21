#include "meditorarea.h"

#include "../../../util/mutil.h"
#include "ui_meditorarea.h"

MEditorArea::MEditorArea(QWidget* parent)
    : QWidget(parent), ui(new Ui::MEditorArea) {
  ui->setupUi(this);
  // 连接画布时间更新信号
  connect(ui->canvas, &MapWorkspaceCanvas::current_time_stamp_changed, this,
          &MEditorArea::on_canvas_timestamp_changed);

  // 默认隐藏音频控制器
  ui->audio_time_controller->hide();
}

MEditorArea::~MEditorArea() { delete ui; }

// 使用主题
void MEditorArea::use_theme(GlobalTheme theme) {
  current_theme = theme;
  QColor file_button_color;
  switch (theme) {
    case GlobalTheme::DARK: {
      file_button_color = QColor(255, 255, 255);
      break;
    }
    case GlobalTheme::LIGHT: {
      file_button_color = QColor(0, 0, 0);
      break;
    }
  }

  // 设置工具栏按钮图标颜色
  mutil::set_toolbutton_svgcolor(ui->default_divisor_policy_toolbutton,
                                 ":/icons/stream.svg", file_button_color, 12,
                                 12);
  mutil::set_toolbutton_svgcolor(ui->bookmark_toolbutton,
                                 ":/icons/bookmark.svg", file_button_color, 12,
                                 12);
  mutil::set_button_svgcolor(ui->audio_time_controller_button,
                             ":/icons/sliders-h.svg", file_button_color, 16,
                             16);
  mutil::set_button_svgcolor(ui->magnet_todivisor_button, ":/icons/magnet.svg",
                             file_button_color, 16, 16);

  // 两个状态
  mutil::set_button_svgcolor(ui->wheel_direction_button,
                             ":/icons/long-arrow-alt-up.svg", file_button_color,
                             16, 16);
  mutil::set_button_svgcolor(ui->lock_edit_mode_button, ":/icons/lock-open.svg",
                             file_button_color, 16, 16);
}

// 画布时间变化事件
void MEditorArea::on_canvas_timestamp_changed(double time) {
  double t = 0.0;
  if (ui->canvas->working_map) {
    // 更新进度条
    auto maptime = (double)(ui->canvas->working_map->map_length);
    // XINFO("src maptime: " +
    // std::to_string(ui->canvas->working_map->map_length)); XINFO("maptime: " +
    // std::to_string(maptime));
    double ratio = time / maptime;
    // XINFO("ratio: " + std::to_string(ratio));
    t = ratio * 10000.0;
  }
  ui->progress_slider->setValue(int(t));
}

// page选择了新map事件
void MEditorArea::on_selectnewmap(std::shared_ptr<MMap>& map) {
  // 为画布切换map
  ui->canvas->switch_map(map);
  // 更新一下
  ui->canvas->repaint();
}

// 滚动方向切换按钮触发
void MEditorArea::on_wheel_direction_button_toggled(bool checked) {
  use_natural_wheel = checked;
  // 根据主题切换图标颜色
  QColor file_button_color;
  switch (current_theme) {
    case GlobalTheme::DARK: {
      file_button_color = QColor(255, 255, 255);
      break;
    }
    case GlobalTheme::LIGHT: {
      file_button_color = QColor(0, 0, 0);
      break;
    }
  }

  // 两个状态
  mutil::set_button_svgcolor(ui->wheel_direction_button,
                             (checked ? ":/icons/long-arrow-alt-down.svg"
                                      : ":/icons/long-arrow-alt-up.svg"),
                             file_button_color, 16, 16);
}

// 吸附到分拍线按钮状态切换事件
void MEditorArea::on_magnet_todivisor_button_toggled(bool checked) {}

// 模式锁定按钮状态切换事件
void MEditorArea::on_lock_edit_mode_button_toggled(bool checked) {
  lock_mode_auto_switch = checked;
  // 根据主题切换图标颜色
  QColor file_button_color;
  switch (current_theme) {
    case GlobalTheme::DARK: {
      file_button_color = QColor(255, 255, 255);
      break;
    }
    case GlobalTheme::LIGHT: {
      file_button_color = QColor(0, 0, 0);
      break;
    }
  }

  // 两个状态
  mutil::set_button_svgcolor(
      ui->lock_edit_mode_button,
      (checked ? ":/icons/lock.svg" : ":/icons/lock-open.svg"),
      file_button_color, 16, 16);
}

// 进度条移动事件
void MEditorArea::on_progress_slider_valueChanged(int value) {
  // 同步画布时间
  if (ui->canvas->is_paused()) {
    // 不暂停不允许调节时间
    if (ui->canvas->working_map) {
      double ratio = (double)value / 10000.0;
      auto maptime = (double)(ui->canvas->working_map->map_length);
      // XINFO("maptime: " + std::to_string(maptime));
      // XINFO("ratio: " + std::to_string(ratio));
      //  qDebug() << "value:" << value;
      ui->canvas->current_time_stamp = maptime * ratio;
    }
  }
}

// 切换音频控制器
void MEditorArea::on_audio_time_controller_button_clicked() {
  ui->audio_time_controller->setHidden(!ui->audio_time_controller->isHidden());
}
