#ifndef MMM_PROJECTSERVICE_HPP
#define MMM_PROJECTSERVICE_HPP

#include <QObject>
#include <mmm/project/MProject.hpp>
#include <string>
#include <unordered_map>
#include <util/StringHash.hpp>

class MMap;
class MapCanvas;
class TrackManager;

class ProjectService : public QObject {
    Q_OBJECT
   public slots:
    void onOpenProject(std::string_view project_path);

   public:
   signals:
    void activateProject(MProject* activated_project) const;
    void activateMap(MProject* activated_project, MMap* map);

   public:
    // 构造ProjectService
    explicit ProjectService(MapCanvas* canvas, TrackManager* trackmanager,
                            QObject* parent = nullptr);
    // 析构ProjectService
    ~ProjectService() override;

    void selectProject(std::string_view project_path);
    void selectMap(std::string_view project_path, MMap* map);

    MProject* currentPorject();

   private:
    // 管理所有项目的内存-因为项目释放时需要使用gl上下文释放纹理
    std::unordered_map<std::string, std::unique_ptr<MProject>, StringHash,
                       std::equal_to<>>
        projects;

    MProject* current_selected_porject{nullptr};

    // 绑定的画布上下文
    MapCanvas* map_canvas{nullptr};

    // 绑定的音轨管理器上下文
    TrackManager* track_manager{nullptr};
};
#endif  // MMM_PROJECTSERVICE_HPP
