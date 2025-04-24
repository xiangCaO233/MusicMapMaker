#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <qobject.h>
#include <qtmetamacros.h>

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

  // 使用主题
  void use_theme(GlobalTheme theme);

 public slots:
  // 更新窗口标题
  void update_window_title(QString &suffix);

 private:
  Ui::MainWindow *ui;
};
#endif  // MAINWINDOW_H
