#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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
  // 显示欢迎页
  bool show_hello_page{true};
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
  MapWorkspaceCanvas *canvas;
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override;

  // 使用真实gl上下文的画布
  MapWorkspaceCanvas *canvas_context = nullptr;

  // 欢迎页
  HelloUserPage *hello_page;

  // 全部设置
  Settings settings;

  // 上一次的标签页索引
  int32_t last_main_tab_index{0};

  // 打开的谱面映射表
  std::unordered_map<std::string, std::shared_ptr<MMap>> opened_maps_map;

  // 标签页索引的谱面映射表
  std::unordered_map<int32_t, std::shared_ptr<MMap>> tabindex_maps_map;

  // 谱面的标签页索引映射表
  std::unordered_map<std::shared_ptr<MMap>, int32_t> maps_tabindex_map;

  // 使用自然滚动
  bool use_natural_wheel{false};

  // 音频管理器
  std::shared_ptr<XAudioManager> audio_manager;

  // 当前选择的音频输出设备
  std::shared_ptr<XOutputDevice> current_use_device;

  // 当前主题
  GlobalTheme current_theme;

  // 文件浏览器控制器
  std::shared_ptr<MFileBrowserController> filebrowercontroller;

  // 使用主题
  void use_theme(GlobalTheme theme);

 private slots:
  // 文件浏览器上下文菜单事件
  void on_file_browser_treeview_customContextMenuRequested(const QPoint &pos);
  // 选择音频输出设备事件
  void on_audio_device_selector_currentIndexChanged(int index);

  // 项目控制器选择了map事件
  void project_controller_select_map(std::shared_ptr<MMap> &map);

  // 滚动方向切换按钮触发
  void on_wheel_direction_button_toggled(bool checked);

  // 切换当前标签页事件
  void on_main_tabs_widget_currentChanged(int index);

 private:
  Ui::MainWindow *ui;
};
#endif  // MAINWINDOW_H
