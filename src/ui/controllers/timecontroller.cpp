#include "timecontroller.h"

#include <qdir.h>
#include <qlogging.h>
#include <qtmetamacros.h>

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
    auto file = QDir(binding_map->audio_file_abs_path);
    if (pause) {
      BackgroundAudio::pause_audio(binding_map->project_reference->devicename,
                                   file.canonicalPath().toStdString());
    } else {
      BackgroundAudio::play_audio(binding_map->project_reference->devicename,
                                  file.canonicalPath().toStdString());
    }
    // 尝试添加回调
    BackgroundAudio::add_playpos_callback(
        binding_map->project_reference->devicename,
        file.canonicalPath().toStdString(), binding_map->audio_pos_callback);
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
              file.canonicalPath().toStdString(),
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
  // 修改timeedit可用状态
  // 暂停时不可使用
  // ui->lineEdit->setEnabled(pause);

  // 更新暂停按钮图标
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
  auto file = QDir(binding_map->audio_file_abs_path);
  // 更新音频播放位置
  if (binding_map &&
      binding_map->project_reference->devicename != "unknown output device") {
    BackgroundAudio::set_audio_pos(binding_map->project_reference->devicename,
                                   file.canonicalPath().toStdString(), time);
  }
}

// 变速设置
void TimeController::on_doubleSpinBox_valueChanged(double arg1) {
  // TODO(xiang 2025-04-26): 实现功能
  auto speed_value = arg1 / 100.0;
  if (!ui->enablepitchaltbutton->isChecked()) {
    // 非变速可以实时调整
    // 为音频应用变速(根据是否启用变调)
    if (binding_map->project_reference->devicename == "unknown output device")
      return;
    BackgroundAudio::set_play_speed(binding_map->project_reference->devicename,
                                    speed_value,
                                    ui->enablepitchaltbutton->isChecked());
    // 同步画布的时间倍率
    emit playspeed_changed(speed_value);
  }
}

// 变速设置完成
void TimeController::on_doubleSpinBox_editingFinished() {
  auto speed_value = ui->doubleSpinBox->value() / 100.0;
  qDebug() << "finished:" << speed_value;
  if (ui->enablepitchaltbutton->isChecked()) {
    // 变速不可以实时调整
    // 为音频应用变速(根据是否启用变调)
    if (binding_map->project_reference->devicename == "unknown output device")
      return;
    BackgroundAudio::set_play_speed(binding_map->project_reference->devicename,
                                    speed_value,
                                    ui->enablepitchaltbutton->isChecked());
    // 同步画布的时间倍率
    emit playspeed_changed(speed_value);
  }
}

// 重置变速按钮事件
void TimeController::on_resetspeedbutton_clicked() {
  // TODO(xiang 2025-04-26): 实现功能
  // 重置音频速度为1.0
  // 同步画布的时间倍率
}

// 启用变速变调按钮事件
void TimeController::on_enablepitchaltbutton_clicked() {
  enable_pitch_alt = ui->enablepitchaltbutton->isChecked();
  BackgroundAudio::enable_pitch_alt = enable_pitch_alt;
}

// 全局音频音量slider值变化事件
void TimeController::on_global_volume_slider_valueChanged(int value) {
  BackgroundAudio::global_volume = float(value) / 100.0;
}

// 音乐音量slider值变化事件
void TimeController::on_music_volume_slider_valueChanged(int value) {
  BackgroundAudio::music_orbits_volume = float(value) / 100.0;
}

// 效果音量slider值变化事件
void TimeController::on_effect_volume_slider_valueChanged(int value) {
  BackgroundAudio::effect_orbits_volume = float(value) / 100.0;
}

// 静音全局按钮事件
void TimeController::on_reset_global_volume_button_clicked() {
  BackgroundAudio::global_volume = 0;
}

// 静音音乐按钮事件
void TimeController::on_reset_music_volume_button_clicked() {
  BackgroundAudio::music_orbits_volume = 0;
}

// 静音效果按钮事件
void TimeController::on_reset_effect_volume_button_clicked() {
  BackgroundAudio::effect_orbits_volume = 0;
}

// 快退按钮事件
void TimeController::on_fastbackward_clicked() {
  // TODO(xiang 2025-04-26): 实现功能
  // 快退5s--并同步画布时间
}

// 快进按钮事件
void TimeController::on_fastforward_clicked() {
  // TODO(xiang 2025-04-26): 实现功能
  // 快进5s--并同步画布时间
}

// 时间编辑框回车按下事件
void TimeController::on_lineEdit_returnPressed() {
  // TODO(xiang 2025-04-26): 实现功能
  // 预先记录修改前的数据
  // 检查输入数据是否合法--不合法恢复缓存数据
}
