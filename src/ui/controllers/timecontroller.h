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
  void pause_button_changed(bool paused);

 private slots:
  // 暂停按钮
  void on_pausebutton_clicked();

 private:
  Ui::audio_time_controller *ui;
};

#endif  // TIMECONTROLLER_H
