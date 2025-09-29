#include <audio/track/trackmanager.h>
#include <project/projectmanager.h>

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
    connect(
        FileActionHandler::instance(), &FileActionHandler::open,
        [service, parent]() {
            // 使用文件夹选择器选择项目的目录
            auto file = mutil::getOpenFile(
                qobject_cast<ProjectManager*>(parent), tr("select file"),
                {{tr("Audio File"), ".ogg .mp3 .wav"},
                 {tr("Map File"), ".imd .mmm .osu"}},
                QDir::homePath());
            if (!file.isEmpty()) {
                auto file_path = std::filesystem::path(file.toStdString());
                XINFO(std::format("打开文件:[{}]", file_path.generic_string()));
                auto parent_dir_path = file_path.parent_path();
                // 将父目录作为项目打开
                service->onOpenProject(parent_dir_path.generic_string());
                service->selectProject(
                    parent_dir_path.filename().generic_string());
            } else {
                // 取消打开
                XINFO("取消打开文件");
            }
        });

    // 连接action的打开文件夹操作到此
    connect(
        FileActionHandler::instance(), &FileActionHandler::open_directory,
        [service, parent]() {
            // 使用文件夹选择器选择项目的目录
            auto dir = mutil::getDirectory(
                qobject_cast<ProjectManager*>(parent),
                tr("select project directory"), QDir::homePath());
            if (!dir.isEmpty()) {
                auto ppath = dir.toStdString();
                XINFO(std::format("打开路径:[{}]", ppath));
                // 打开项目
                service->onOpenProject(ppath);
                service->selectProject(
                    std::filesystem::path(ppath).filename().generic_string());
            } else {
                // 取消打开
                XINFO("取消打开文件夹");
            }
        });
}

// 析构ProjectService
ProjectService::~ProjectService() {
    for (const auto& [name, project] : projects) {
        project->close();
    }
}

void ProjectService::selectProject(std::string_view project_path) {
    auto it = projects.find(project_path);
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
        // 项目是否已打开过
        MProject* project;
        if (auto it = projects.find(project_path.generic_string());
            it != projects.end()) {
            project = it->second.get();
        } else {
            // 创建并将项目加入集合
            project = projects
                          .try_emplace(
                              project_path.generic_string(),
                              std::make_unique<MProject>(
                                  map_canvas->textureCallback(), track_manager))
                          .first->second.get();
        }
        // 打开项目
        project->open(project_path.generic_string());
        // 立马选中
        selectProject(project_path.generic_string());
    }
}
