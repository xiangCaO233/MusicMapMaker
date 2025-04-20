#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <qobject.h>

#include <QComboBox>
#include <QMainWindow>
#include <memory>

#include "GlobalSettings.h"

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

  // 音频管理器
  std::shared_ptr<XAudioManager> audio_manager;

  // 当前选择的音频输出设备
  std::shared_ptr<XOutputDevice> current_use_device;

  // 使用主题
  void use_theme(GlobalTheme theme);

 private slots:
  // 选择音频输出设备事件
  void on_audio_device_selector_currentIndexChanged(int index);

 private:
  Ui::MainWindow *ui;
};
#endif  // MAINWINDOW_H
