#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QComboBox>
#include <QMainWindow>
#include <memory>

#include "src/filebrowsercontroller.h"

// 默认主题
enum class GlobalTheme {
  DARK,
  LIGHT,
};

class XAudioManager;
class MapWorkspaceCanvas;
class XOutputDevice;
class MMap;

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

 private:
  Ui::MainWindow *ui;
};
#endif  // MAINWINDOW_H
