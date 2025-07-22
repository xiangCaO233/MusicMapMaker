#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class ProjectManager;
class TrackManager;

class MainWindow : public QMainWindow {
    Q_OBJECT

   public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

   private slots:
    void on_actionTrack_Manager_toggled(bool arg1);

    void on_actionProject_Manager_toggled(bool arg1);

    // 响应管理器关闭事件
    void trackmanager_close_slot();

    void projectmanager_close_slot();

   private:
    Ui::MainWindow *ui;

    // 音轨管理器
    TrackManager *trackmanager;

    // 项目管理器
    ProjectManager *projectmanager;

    // 更新标题后缀
    void update_title_suffix(const QString &suffix);
};

#endif  // MAINWINDOW_H
