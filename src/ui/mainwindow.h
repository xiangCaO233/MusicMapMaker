#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <edit/mapeditor.h>

#include <QMainWindow>

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

   private slots:
    void on_actionTrack_Manager_toggled(bool arg1);

    void on_actionProject_Manager_toggled(bool arg1);

    void on_actionEditor_toggled(bool arg1);

   protected:
    void closeEvent(QCloseEvent *e) override;

   private:
    Ui::MainWindow *ui;

    // 编辑器
    MapEditor *editor;

    // 音轨管理器
    TrackManager *trackmanager;

    // 项目管理器
    ProjectManager *projectmanager;
};

#endif  // MAINWINDOW_H
