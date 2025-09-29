#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <edit/mapeditor.h>

#include <GlobalSettings.hpp>
#include <QMainWindow>

namespace Ui {
class MainWindow;
}

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
    // void on_actionTrack_Manager_toggled(bool arg1);

    // void on_actionProject_Manager_toggled(bool arg1);

    // 更新标题后缀
    void update_title_suffix(const QString &suffix);

   protected:
    void closeEvent(QCloseEvent *e) override;

   private:
    Ui::MainWindow *ui;

    // 初始化所有信号连接
    void connectAll();

    // 初始化所有的action
    void initActions();
};

#endif  // MAINWINDOW_H
