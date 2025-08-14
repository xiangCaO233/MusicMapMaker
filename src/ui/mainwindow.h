#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <edit/mapeditor.h>

#include <QMainWindow>

#include "GlobalSettings.hpp"

namespace Ui {
class MainWindow;
}

class ProjectManager;
class HideableToolWindow;
class TrackManager;

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

    // 使用主题
    void use_theme(GlobalTheme theme);

   private slots:
    void on_actionTrack_Manager_toggled(bool arg1);

    void on_actionProject_Manager_toggled(bool arg1);

    // 更新标题后缀
    void update_title_suffix(const QString &suffix);

    // 默认皮肤初始化完成
    void onDefSkinInitialized();

   protected:
    void closeEvent(QCloseEvent *e) override;

   private:
    Ui::MainWindow *ui;
};

#endif  // MAINWINDOW_H
