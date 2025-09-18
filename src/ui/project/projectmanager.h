#ifndef PROJECTMANAGER_H
#define PROJECTMANAGER_H

#include <projectconfig.h>

#include <ProjectService.hpp>
#include <QStandardItem>
#include <QWidget>

namespace Ui {
class ProjectManager;
}

class MapCanvas;
class TrackManager;

class ProjectManager : public QWidget {
    Q_OBJECT

   public:
    explicit ProjectManager(QWidget *parent = nullptr);
    ~ProjectManager() override;

    // 初始化管理器
    void initService(MapCanvas *canvas, TrackManager *trackmanager);

   signals:
    void openProject(std::string_view project_path);
    void closeProject(std::string_view project_name);

   protected:
    void closeEvent(QCloseEvent *e) override;

   public slots:
    void onMapCanvasThreadStopped();
   private slots:
    // void on_create_project_button_clicked();
    // void on_add_project_button_clicked();
    // void on_close_project_button_clicked();
    // void on_project_list_doubleClicked(const QModelIndex &index);

    void on_map_listView_doubleClicked(const QModelIndex &index);

    // void on_preference_button_clicked();

    // void onUpdateProjectListView(
    //     const std::unordered_map<std::string, std::unique_ptr<MProject>,
    //                              StringHash, std::equal_to<>> *projects)
    //                              const;

    void onActivateProject(MProject *activated_project);

   private:
    // 项目服务
    ProjectService *service{nullptr};

    // 项目配置界面
    ProjectConfig *config_ui;

    Ui::ProjectManager *ui;
};

#endif  // PROJECTMANAGER_H
