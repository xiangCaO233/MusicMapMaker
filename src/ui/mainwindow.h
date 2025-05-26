#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <qevent.h>
#include <qobject.h>
#include <qtmetamacros.h>

#include <QComboBox>
#include <QMainWindow>
#include <memory>

#include "../log/uilogger.h"
#include "GlobalSettings.h"
#include "mpage.h"

class MapWorkspaceCanvas;
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

    // 全局样式表
    static QString global_style_sheet;

    // 全部设置
    static Settings settings;

    // 当前主题
    GlobalTheme current_theme;

    // page
    MPage *page;

    // 使用主题
    void use_theme(GlobalTheme theme);

    void init_actions();

   public slots:
    // 更新窗口标题
    void update_window_title(QString &suffix);

   private:
    Ui::MainWindow *ui;
};
#endif  // MAINWINDOW_H
