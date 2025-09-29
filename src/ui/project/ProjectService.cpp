#include <audio/track/trackmanager.h>

#include <action/modules/file/FileActionHandler.hpp>
#include <canvas/map/MapCanvas.hpp>
#include <memory>
#include <mmm/project/MProject.hpp>
#include <project/ProjectService.hpp>

// 构造ProjectService
ProjectService::ProjectService(MapCanvas* canvas, TrackManager* trackmanager,
                               QObject* parent)
    : QObject(parent) {
    map_canvas = canvas;
    track_manager = trackmanager;
    auto service = this;

    // 连接action的打开文件操作到此
    connect(FileActionHandler::instance(), &FileActionHandler::open,
            [service](std::string_view dir) {
                // service->onOpenProject(dir);
                // service->selectProject(
                //     std::filesystem::path(dir).filename().generic_string());
            });

    // 连接action的打开文件夹操作到此
    connect(FileActionHandler::instance(), &FileActionHandler::open_directory,
            [service](std::string dir) {
                service->onOpenProject(dir);
                service->selectProject(
                    std::filesystem::path(dir).filename().generic_string());
            });
}

// 析构ProjectService
ProjectService::~ProjectService() {
    for (const auto& [name, project] : projects) {
        project->close();
    }
}

void ProjectService::selectProject(std::string_view project_name) {
    auto it = projects.find(project_name);
    if (it != projects.end()) {
        current_selected_porject = it->second.get();
        emit activateProject(current_selected_porject);
    }
}
MProject* ProjectService::currentPorject() { return current_selected_porject; }

void ProjectService::selectMap(std::string_view current_project_name,
                               MMap* map) {
    auto pit = projects.find(current_project_name);
    if (pit != projects.end()) {
        map_canvas->switch_map(map);
        emit activateMap(pit->second.get(), map);
    }
}

void ProjectService::onOpenProject(std::string_view path) {
    if (std::filesystem::path project_path(path);
        !std::filesystem::exists(project_path) ||
        !std::filesystem::is_directory(project_path)) {
        return;
    } else {
        // 打开的项目是否是mproject
        bool is_mproject{false};
        auto project = std::make_unique<MProject>(map_canvas->textureCallback(),
                                                  track_manager);
        // 打开项目
        project->open(project_path.generic_string());

        // 目标项目名称(作为项目集合的key)
        auto project_name = project->cfg()->project_name;

        // 将项目加入集合
        projects.try_emplace(project_name, std::move(project))
            .first->second.get();
    }
    // 发送更新项目列表信号
    emit updateProjectList(&projects);
}

void ProjectService::onCloseProject(std::string_view project_name) {
    auto it = projects.find(project_name);
    if (it == projects.end()) {
        qDebug() << "不存在项目[" << project_name << "]";
        return;
    }
    projects.erase(it);
    // 发送更新列表信号
    emit updateProjectList(&projects);
}
