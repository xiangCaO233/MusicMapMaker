#ifndef TIMECONTROLLER_H
#define TIMECONTROLLER_H

#include <qtmetamacros.h>

#include <QWidget>
#include <memory>

class MMap;
enum class GlobalTheme;

namespace Ui {
class audio_time_controller;
}

class TimeController : public QWidget {
  Q_OBJECT

 public:
  explicit TimeController(QWidget *parent = nullptr);
  ~TimeController();

  // 当前主题
  GlobalTheme current_theme;

  // 暂停状态
  bool pause{true};

  // 变调是否启用
  bool enable_pitch_alt{false};

  // 时间控制器绑定的map
  std::shared_ptr<MMap> binding_map;

  // 使用主题
  void use_theme(GlobalTheme theme);

  // 更新全局音量按钮(主题)
  void update_global_volume_button();

  // 更新音频状态
  void update_audio_status();

  // 更新暂停按钮
  void update_pause_button();

 public slots:
  // page选择了新map事件
  void on_selectnewmap(std::shared_ptr<MMap> &map);

  // 画布时间变化事件
  void on_canvas_timestamp_changed(double time);

  // 画布暂停槽
  void on_canvas_pause(bool paused);

  // 实时信息变化槽
  void on_current_bpm_changed(double bpm);
  void on_current_timeline_speed_changed(double timeline_speed);

 signals:
  // 时间控制器编辑时间信号
  void time_edited(double time);

  // 时间控制器暂停信号
  void pause_button_changed(bool paused);

  // 时间控制器播放速度变化信号
  void playspeed_changed(double speed);

 private slots:
  // 暂停按钮
  void on_pausebutton_clicked();

  // 变速设置
  void on_doubleSpinBox_valueChanged(double arg1);

  // 重置变速按钮事件
  void on_resetspeedbutton_clicked();

  // 启用变速变调按钮事件
  void on_enablepitchaltbutton_clicked();

  // 全局音频音量slider值变化事件
  void on_global_volume_slider_valueChanged(int value);

  // 音乐音量slider值变化事件
  void on_music_volume_slider_valueChanged(int value);

  // 效果音量slider值变化事件
  void on_effect_volume_slider_valueChanged(int value);

  // 静音全局按钮事件
  void on_reset_global_volume_button_clicked();

  // 静音音乐按钮事件
  void on_reset_music_volume_button_clicked();

  // 静音效果按钮事件
  void on_reset_effect_volume_button_clicked();

  // 快退按钮事件
  void on_fastbackward_clicked();

  // 快进按钮事件
  void on_fastforward_clicked();

  // 时间编辑框回车按下事件
  void on_lineEdit_returnPressed();

 private:
  Ui::audio_time_controller *ui;
};

#endif  // TIMECONTROLLER_H
