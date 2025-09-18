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

        // 目标项目名称(作为项目集合的key)
        std::string project_name = project_path.filename().generic_string();

        // 寻找项目配置(仅在项目根目录下)
        for (auto it = std::filesystem::directory_iterator(project_path);
             it != std::filesystem::directory_iterator(); ++it) {
            auto filename_full = it->path().filename().generic_string();
            if (filename_full.ends_with(".mproject")) {
                // 读取项目配置

                // 顺利读取项目配置-确认这是mproject
                is_mproject = true;
            }
        }
        if (is_mproject) {
            // 并非mproject-询问是否初始化该目录为mproject目录

            // 重新获得project_name
        }

        // 创建项目加入集合
        auto project =
            projects
                .try_emplace(project_name,
                             std::make_unique<MProject>(
                                 map_canvas->textureCallback(), track_manager))
                .first->second.get();

        // 打开项目
        project->open(project_path.generic_string());
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
