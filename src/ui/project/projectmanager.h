#ifndef PROJECTMANAGER_H
#define PROJECTMANAGER_H

#include <config/project/projectconfig.h>

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
    void initService(MapCanvas *canvas);

    ProjectService *get_service();

   signals:
    void openProject(std::string_view project_path);
    void closeProject(std::string_view project_name);

   protected:
    void closeEvent(QCloseEvent *e) override;

   public slots:
    void onMapCanvasThreadStopped();
    // 默认皮肤初始化完成
    void onDefSkinInitialized();
   private slots:

    void on_map_listView_doubleClicked(const QModelIndex &index);

    void onActivateProject(MProject *activated_project);

    void on_map_listView_clicked(const QModelIndex &index);

   private:
    // 项目服务
    ProjectService *service{nullptr};

    Ui::ProjectManager *ui;
};

#endif  // PROJECTMANAGER_H
