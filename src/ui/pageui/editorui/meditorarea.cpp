#include "meditorarea.h"

#include <qtmetamacros.h>

#include "../../../util/mutil.h"
#include "ui_meditorarea.h"

MEditorArea::MEditorArea(QWidget* parent)
    : QWidget(parent), ui(new Ui::MEditorArea) {
  ui->setupUi(this);
  canvas_container = ui->canvas_container;
  // 默认隐藏音频控制器
  // ui->audio_time_controller->hide();

  // 连接时间控制器选择map槽
  connect(this, &MEditorArea::switched_map, ui->audio_time_controller,
          &TimeController::on_selectnewmap);

  // 连接画布信号和时间控制器暂停槽(来回)
  connect(ui->canvas_container->canvas.data(),
          &MapWorkspaceCanvas::pause_signal, ui->audio_time_controller,
          &TimeController::on_canvas_pause);
  connect(ui->audio_time_controller, &TimeController::pause_button_changed,
          ui->canvas_container->canvas.data(),
          &MapWorkspaceCanvas::on_timecontroller_pause_button_changed);

  // 连接画布槽和时间控制器信号
  connect(ui->audio_time_controller, &TimeController::on_canvas_pause,
          canvas_container->canvas.data(),
          &MapWorkspaceCanvas::on_timecontroller_pause_button_changed);

  connect(ui->audio_time_controller, &TimeController::playspeed_changed,
          canvas_container->canvas.data(),
          &MapWorkspaceCanvas::on_timecontroller_speed_changed);

  // 连接画布时间更新信号
  connect(ui->canvas_container->canvas.data(),
          &MapWorkspaceCanvas::current_time_stamp_changed, this,
          &MEditorArea::on_canvas_timestamp_changed);
  connect(ui->canvas_container->canvas.data(),
          &MapWorkspaceCanvas::current_time_stamp_changed,
          ui->audio_time_controller,
          &TimeController::on_canvas_timestamp_changed);

  // 连接bpm,时间线速度变化槽
  connect(ui->canvas_container->canvas.data(),
          &MapWorkspaceCanvas::current_absbpm_changed,
          ui->audio_time_controller, &TimeController::on_current_bpm_changed);
  connect(ui->canvas_container->canvas.data(),
          &MapWorkspaceCanvas::current_timeline_speed_changed,
          ui->audio_time_controller,
          &TimeController::on_current_timeline_speed_changed);

  // 时间控制器-进度条
  connect(this, &MEditorArea::progress_pos_changed, ui->audio_time_controller,
          &TimeController::on_canvas_timestamp_changed);
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
  mutil::set_toolbutton_svgcolor(ui->mode_toolbutton,
                                 ":/icons/mouse-pointer.svg", file_button_color,
                                 12, 12);

  mutil::set_button_svgcolor(ui->magnet_todivisor_button, ":/icons/magnet.svg",
                             file_button_color, 16, 16);

  // 两个状态
  mutil::set_button_svgcolor(ui->wheel_direction_button,
                             ":/icons/long-arrow-alt-up.svg", file_button_color,
                             16, 16);
  mutil::set_button_svgcolor(ui->lock_edit_mode_button, ":/icons/lock-open.svg",
                             file_button_color, 16, 16);

  // 设置时间控制器主题
  ui->audio_time_controller->use_theme(theme);
}

// 画布时间变化事件
// 更新进度条
void MEditorArea::on_canvas_timestamp_changed(double time) {
  double t = 0.0;
  if (ui->canvas_container->canvas.data()->working_map) {
    // 更新进度条
    auto maptime =
        (double)(ui->canvas_container->canvas.data()->working_map->map_length);
    double ratio = time / maptime;
    t = ratio * 10000.0;
  }
  sync_time_lock = true;
  ui->progress_slider->setValue(int(t));
}

// page选择了新map事件
void MEditorArea::on_selectnewmap(std::shared_ptr<MMap>& map) {
  // 为画布切换map
  ui->canvas_container->canvas.data()->switch_map(map);

  // 发送个信号
  emit switched_map(map);

  // 更新一下
  ui->canvas_container->canvas.data()->update();
}

// 滚动方向切换按钮触发
void MEditorArea::on_wheel_direction_button_toggled(bool checked) {
  ui->canvas_container->canvas.data()->scroll_direction =
      (checked ? -1.0 : 1.0);

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
void MEditorArea::on_magnet_todivisor_button_toggled(bool checked) {
  ui->canvas_container->canvas.data()->magnet_to_divisor = checked;
}

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
  if (sync_time_lock) {
    sync_time_lock = false;
    return;
  }
  // 同步画布时间
  if (ui->canvas_container->canvas.data()->is_paused()) {
    // 不暂停不允许调节时间
    if (ui->canvas_container->canvas.data()->working_map) {
      double ratio = (double)value / 10000.0;
      auto maptime = (double)(ui->canvas_container->canvas.data()
                                  ->working_map->map_length);
      ui->canvas_container->canvas.data()->current_time_stamp = maptime * ratio;
      ui->canvas_container->canvas.data()->current_visual_time_stamp =
          ui->canvas_container->canvas.data()->current_time_stamp +
          ui->canvas_container->canvas.data()->static_time_offset;
      emit progress_pos_changed(maptime * ratio);
    }
  }
}

// 切换音频控制器
// void MEditorArea::on_audio_time_controller_button_clicked() {
//   ui->audio_time_controller->setHidden(!ui->audio_time_controller->isHidden());
// }
