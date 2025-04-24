#include "timecontroller.h"

#include <functional>
#include <memory>
#include <string>

#include "../../log/colorful-log.h"
#include "../../mmm/MapWorkProject.h"
#include "../../mmm/map/MMap.h"
#include "../../util/mutil.h"
#include "../GlobalSettings.h"
#include "AudioManager.h"
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

// 画布暂停槽
void TimeController::on_canvas_pause(bool paused) {
  // 更新暂停状态和按钮图标
  pause = paused;
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
  // 切换音频播放状态
  if (binding_map) {
    // 检查项目使用的音频设备
    auto &device = binding_map->project_reference->device;
    if (device) {
      // 在播放器查找对应音频轨道
      std::shared_ptr<XAudioOrbit> orbit;
      auto orbitit = device->player->mixer->audio_orbits.find(
          binding_map->audio_reference->handle);
      if (orbitit == device->player->mixer->audio_orbits.end()) {
        // 新建轨道
        orbit = std::make_shared<XAudioOrbit>(binding_map->audio_reference);
        device->player->mixer->add_orbit(orbit);
      } else {
        orbit = orbitit->second;
      }
      // 应用暂停状态
      orbit->paused = pause;
      // 更新播放器状态
      if (pause) {
        device->player->pause();
      } else {
        device->player->resume();
      }
    }
  }
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
  if (!pause) return;
  // 更新音频播放位置
  if (binding_map) {
    // 检查项目使用的音频设备
    auto &device = binding_map->project_reference->device;
    if (device) {
      audio_manager_reference->set_audio_current_pos(
          audio_manager_reference->get_outdevice_indicies()->at(
              device->device_name),
          binding_map->audio_reference->handle, time);
    }
  }
}
