#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <qobject.h>

#include <QComboBox>
#include <QMainWindow>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include "src/filebrowsercontroller.h"

// 默认主题
enum class GlobalTheme {
  DARK,
  LIGHT,
};

// 设置
struct Settings {
  // 主题
  GlobalTheme current_theme;
};

class XAudioManager;
class MapWorkspaceCanvas;
class XOutputDevice;
class MMap;
class HelloUserPage;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override;

  // 全部设置
  Settings settings;

  // 当前主题
  GlobalTheme current_theme;

  // page名的谱面映射表
  std::unordered_map<QString, std::shared_ptr<MMap>> pagetext_maps_map;

  // 使用自然滚动
  bool use_natural_wheel{false};

  // 锁定模式-不自动切换
  bool lock_mode_auto_switch{false};

  // 音频管理器
  std::shared_ptr<XAudioManager> audio_manager;

  // 当前选择的音频输出设备
  std::shared_ptr<XOutputDevice> current_use_device;

  // 文件浏览器控制器
  std::shared_ptr<MFileBrowserController> filebrowercontroller;

  // 使用主题
  void use_theme(GlobalTheme theme);

  // 设置信号
  void setupsignals();

 private slots:
  // 文件浏览器上下文菜单事件
  void on_file_browser_treeview_customContextMenuRequested(const QPoint &pos);

  // 选择音频输出设备事件
  void on_audio_device_selector_currentIndexChanged(int index);

  // 项目控制器选择了map事件
  void project_controller_select_map(std::shared_ptr<MMap> &map);

  // 滚动方向切换按钮触发
  void on_wheel_direction_button_toggled(bool checked);

  // 模式锁定按钮状态切换事件
  void on_lock_edit_mode_button_toggled(bool checked);

  // 选择页事件
  void on_page_selector_currentTextChanged(const QString &text);

  // 关闭页面事件
  void on_close_page_button_clicked();

  // 进度条移动事件
  void on_progress_slider_valueChanged(int value);

  // 画布时间变化事件
  void on_canvas_timestamp_changed(double time);

 private:
  Ui::MainWindow *ui;
};
#endif  // MAINWINDOW_H
