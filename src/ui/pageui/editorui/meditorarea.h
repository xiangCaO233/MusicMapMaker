#ifndef MEDITORAREA_H
#define MEDITORAREA_H

#include <qtmetamacros.h>

#include <QWidget>
#include <memory>

#include "../../GlobalSettings.h"

class MMap;
class CanvasContainer;

namespace Ui {
class MEditorArea;
}

class MEditorArea : public QWidget {
  Q_OBJECT

 public:
  explicit MEditorArea(QWidget *parent = nullptr);
  ~MEditorArea();

  // 画布容器引用
  CanvasContainer *canvas_container;

  // 当前主题
  GlobalTheme current_theme;

  // 锁定模式-不自动切换
  bool lock_mode_auto_switch{false};

  // 使用主题
  void use_theme(GlobalTheme theme);

  // 同步时间锁
  bool sync_time_lock{false};

 public slots:
  // page选择了新map事件
  void on_selectnewmap(std::shared_ptr<MMap> &map);
 signals:
  // 切换map信号
  void switched_map(std::shared_ptr<MMap> &map);

  // 进度条移动信号
  void progress_pos_changed(double ratio);

 private slots:
  // 画布时间变化事件
  void on_canvas_timestamp_changed(double time);

  // 滚动方向切换按钮触发
  void on_wheel_direction_button_toggled(bool checked);

  // 吸附到分拍线按钮状态切换事件
  void on_magnet_todivisor_button_toggled(bool checked);

  // 模式锁定按钮状态切换事件
  void on_lock_edit_mode_button_toggled(bool checked);

  // 进度条移动事件
  void on_progress_slider_valueChanged(int value);

  void on_show_object_after_judgeline_button_toggled(bool checked);

 private:
  Ui::MEditorArea *ui;

  void initialize_toolbuttons();
};

#endif  // MEDITORAREA_H
