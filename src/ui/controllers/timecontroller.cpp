#include "timecontroller.h"

#include <qtmetamacros.h>
#include <QDir>

#include <memory>
#include <string>

#include "../../mmm/MapWorkProject.h"
#include "../../mmm/map/MMap.h"
#include "../../util/mutil.h"
#include "../GlobalSettings.h"
#include "AudioManager.h"
#include "audio/BackgroundAudio.h"
#include "colorful-log.h"
#include "ui_timecontroller.h"

TimeController::TimeController(QWidget *parent)
    : QWidget(parent), ui(new Ui::audio_time_controller) {
  ui->setupUi(this);
}

TimeController::~TimeController() { delete ui; }

// 使用主题
void TimeController::use_theme(GlobalTheme theme) {
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

  // 设置按钮图标颜色
  mutil::set_button_svgcolor(ui->pausebutton, ":/icons/play.svg",
                             file_button_color, 16, 16);
  mutil::set_button_svgcolor(ui->fastbackward, ":/icons/backward.svg",
                             file_button_color, 16, 16);
  mutil::set_button_svgcolor(ui->fastforward, ":/icons/forward.svg",
                             file_button_color, 16, 16);
  mutil::set_button_svgcolor(ui->enablepitchaltbutton, ":/icons/pitch-alt.svg",
                             file_button_color, 16, 16);
  mutil::set_button_svgcolor(ui->resetspeedbutton, ":/icons/bolt.svg",
                             file_button_color, 16, 16);

  mutil::set_button_svgcolor(ui->reset_global_volume_button,
                             ":/icons/volume-high.svg", file_button_color, 16,
                             16);
  mutil::set_button_svgcolor(ui->reset_music_volume_button, ":/icons/music.svg",
                             file_button_color, 16, 16);

  mutil::set_button_svgcolor(ui->reset_effect_volume_button,
                             ":/icons/effect.svg", file_button_color, 16, 16);
}

// 更新全局音量按钮(主题)
void TimeController::update_global_volume_button() {
  QColor button_color;
  switch (current_theme) {
    case GlobalTheme::DARK: {
      button_color = QColor(255, 255, 255);
      break;
    }
    case GlobalTheme::LIGHT: {
      button_color = QColor(0, 0, 0);
      break;
    }
  }
  auto current_global_volume = ui->global_volume_slider->value();
  mutil::set_button_svgcolor(
      ui->reset_global_volume_button,
      (current_global_volume > 50
           ? ":/icons/volume-high.svg"
           : (current_global_volume > 0 ? ":/icons/volume-low.svg"
                                        : ":/icons/volume-off.svg")),
      button_color, 16, 16);
}

// 更新音频状态
void TimeController::update_audio_status() {
  if (binding_map) {
    auto s = QDir(binding_map->audio_file_abs_path);
    if (pause) {
      BackgroundAudio::pause_audio(binding_map->project_reference->devicename,
                                   s.canonicalPath().toStdString());
    } else {
      BackgroundAudio::play_audio(binding_map->project_reference->devicename,
                                  s.canonicalPath().toStdString());
    }
    // 尝试添加回调
    BackgroundAudio::add_playpos_callback(
        binding_map->project_reference->devicename,
        s.canonicalPath().toStdString(), binding_map->audio_pos_callback);
    // 暂停的话同步一下
    if (pause) {
      // 防止播放器已播放完暂停了死等待
      if (BackgroundAudio::get_device_playerstatus(
              binding_map->project_reference->devicename) == 1 &&
          !pause) {
        binding_map->audio_pos_callback->waitfor_clear_buffer([&]() {
          XINFO("同步画布时间");
          BackgroundAudio::set_audio_pos(
              binding_map->project_reference->devicename,
              s.canonicalPath().toStdString(),
              binding_map->project_reference->map_canvasposes.at(binding_map));
        });
      }
    }
  }
}

// 更新暂停按钮
void TimeController::update_pause_button() {
  QColor button_color;
  switch (current_theme) {
    case GlobalTheme::DARK: {
      button_color = QColor(255, 255, 255);
      break;
    }
    case GlobalTheme::LIGHT: {
      button_color = QColor(0, 0, 0);
      break;
    }
  }
  mutil::set_button_svgcolor(ui->pausebutton,
                             (pause ? ":/icons/play.svg" : ":/icons/pause.svg"),
                             button_color, 16, 16);
}

// 暂停按钮
void TimeController::on_pausebutton_clicked() {
  pause = !pause;
  update_pause_button();
  // 切换音频播放状态
  update_audio_status();
  emit pause_button_changed(pause);
}

// 画布暂停槽
void TimeController::on_canvas_pause(bool paused) {
  // 更新暂停状态和按钮图标
  pause = paused;
  update_pause_button();

  // 切换音频播放状态
  update_audio_status();
}

// 实时信息变化槽
// bpm
void TimeController::on_current_bpm_changed(double bpm) {
  ui->bpmvalue->setText(QString::number(bpm, 'f', 2));
}

// timeline_speed
void TimeController::on_current_timeline_speed_changed(double timeline_speed) {
  ui->timelinespeedvalue->setText(QString::number(timeline_speed, 'f', 2));
}

// page选择了新map事件
void TimeController::on_selectnewmap(std::shared_ptr<MMap> &map) {
  binding_map = map;
}

// 画布时间变化事件
void TimeController::on_canvas_timestamp_changed(double time) {
  if (!binding_map) return;
  // 计算进度
  auto progress = time / binding_map->map_length;
  // 更新lineedit和progress
  ui->lineEdit->setText(QString::number(int32_t(time)));
  ui->playprogress->setValue(ui->playprogress->maximum() * progress);

  // 暂停可调
  if (!pause) return;
  // 更新音频播放位置
  if (binding_map &&
      binding_map->project_reference->devicename != "unknown output device") {
    auto s = QDir(binding_map->audio_file_abs_path);
    BackgroundAudio::set_audio_pos(binding_map->project_reference->devicename,
                                   s.canonicalPath().toStdString(), time);
  }
}
