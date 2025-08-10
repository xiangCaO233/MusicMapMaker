#ifndef PROJECTMANAGER_H
#define PROJECTMANAGER_H

#include <projectconfig.h>

#include <QStandardItem>
#include <QWidget>
#include <memory>
#include <mmm/project/MProject.hpp>
#include <unordered_map>

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

    // 设置项目需要 绑定的 画布上下文
    void bind_canvas(MapCanvas *canvas);

    // 设置项目需要 绑定的 音轨管理器上下文
    void bind_trackmgr(TrackManager *trackmanager);

    // 打开项目
    void open_project(std::string_view project_path);

    // 关闭项目
    void close_project(std::string_view project_name);

    // 展示项目
    void show_project(std::string_view project_name);

   protected:
    void closeEvent(QCloseEvent *e) override;

   private slots:
    void on_create_project_button_clicked();
    void on_add_project_button_clicked();
    void on_close_project_button_clicked();

    void on_map_listView_doubleClicked(const QModelIndex &index);

    void on_project_list_doubleClicked(const QModelIndex &index);

    void on_preference_button_clicked();

   private:
    // 管理所有项目的内存-因为项目释放时需要使用gl上下文释放纹理
    std::unordered_map<std::string, std::unique_ptr<MProject>, StringHash,
                       std::equal_to<>>
        projects;

    // 项目配置界面
    ProjectConfig *config_ui;

    // 绑定的画布上下文
    MapCanvas *map_canvas{nullptr};

    // 绑定的音轨管理器上下文
    TrackManager *track_manager{nullptr};

    Ui::ProjectManager *ui;
};

#endif  // PROJECTMANAGER_H
