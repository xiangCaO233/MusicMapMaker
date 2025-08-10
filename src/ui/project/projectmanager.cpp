#include "projectmanager.h"

#include <audio/track/trackmanager.h>
#include <qlogging.h>
#include <ui_projectmanager.h>

#include <QCloseEvent>
#include <QStandardItemModel>
#include <canvas/map/MapCanvas.hpp>
#include <filesystem>
#include <mmm/project/MProject.hpp>

#include "projectconfig.h"

ProjectManager::ProjectManager(QWidget* parent)
    : QWidget(parent), ui(new Ui::ProjectManager) {
    ui->setupUi(this);
    ui->main_splitter->setSizes({0, 300});
    ui->project_content_splitter->setSizes({220, 300});

    // 项目列表模型
    auto project_list_model = new QStandardItemModel(ui->project_list);
    ui->project_list->setModel(project_list_model);

    // 谱面列表模型
    auto map_list_model = new QStandardItemModel(ui->map_listView);
    ui->map_listView->setModel(map_list_model);

    // 音频列表模型
    auto audio_list_model = new QStandardItemModel(ui->audio_listView);
    ui->audio_listView->setModel(audio_list_model);

    // 图片列表模型
    auto image_list_model = new QStandardItemModel(ui->image_listView);
    ui->image_listView->setModel(image_list_model);

    // 视频列表模型
    auto video_list_model = new QStandardItemModel(ui->video_listView);
    ui->video_listView->setModel(video_list_model);

    // 初始化项目配置ui
    config_ui = new ProjectConfig();
    config_ui->hide();
}

ProjectManager::~ProjectManager() {
    delete config_ui;
    delete ui;
    qDebug() << "ProjectManager deleted";
}

// 设置项目需要 绑定的 画布上下文
void ProjectManager::bind_canvas(MapCanvas* canvas) { map_canvas = canvas; }

// 设置项目需要 绑定的 音轨管理器上下文
void ProjectManager::bind_trackmgr(TrackManager* trackmanager) {
    track_manager = trackmanager;
}

// 打开项目
void ProjectManager::open_project(std::string_view project_path) {
    auto path = std::filesystem::path(project_path);
    if (!std::filesystem::exists(path) ||
        !std::filesystem::is_directory(path)) {
        return;
    } else {
        // 打开的项目是否是mproject
        bool is_mproject{false};

        // 目标项目名称(作为项目集合的key)
        std::string project_name = path.filename().generic_string();

        // 寻找项目配置(仅在项目根目录下)
        for (auto it = std::filesystem::directory_iterator(path);
             it != std::filesystem::directory_iterator(); ++it) {
            auto filename_full = it->path().generic_string();
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
                .try_emplace(project_name, std::make_unique<MProject>(
                                               map_canvas, track_manager))
                .first->second.get();

        // 打开项目
        project->open(project_path);

        // 新建列表项
        auto project_item =
            new QStandardItem(QString::fromStdString(project_name));
        project_item->setData(QVariant::fromValue(project_name));

        project_item->setEditable(false);

        // 添加列表项
        auto model =
            qobject_cast<QStandardItemModel*>(ui->project_list->model());

        model->appendRow(project_item);
    }
}

// 关闭项目
void ProjectManager::close_project(std::string_view project_name) {
    auto it = projects.find(project_name);
    if (it == projects.end()) {
        qDebug() << "不存在项目[" << project_name << "]";
        return;
    }
    projects.erase(it);
}

// 展示项目
void ProjectManager::show_project(std::string_view project_name) {
    auto it = projects.find(project_name);
    if (it != projects.end()) {
        auto& project = it->second;

        // 更新谱面表
        auto map_model =
            qobject_cast<QStandardItemModel*>(ui->map_listView->model());
        map_model->clear();

        for (const auto& [map_name, map] : project->project_maps_table) {
            auto map_item = new QStandardItem(QString::fromStdString(map_name));
            map_item->setData(QVariant::fromValue(map.get()));
            map_item->setEditable(false);
            map_model->appendRow(map_item);
        }

        // 更新音频表
        auto audio_model =
            qobject_cast<QStandardItemModel*>(ui->audio_listView->model());
        audio_model->clear();
        for (const auto& [audio_name, track] : project->project_audios_table) {
            auto audio_item =
                new QStandardItem(QString::fromStdString(audio_name));
            audio_item->setData(QVariant::fromValue(track));
            audio_item->setEditable(false);
            audio_model->appendRow(audio_item);
        }

        // 更新图片表
        auto image_model =
            qobject_cast<QStandardItemModel*>(ui->image_listView->model());
        image_model->clear();
        for (const auto& image_path : project->project_image_table) {
            auto image_item =
                new QStandardItem(QString::fromStdString(image_path));
            image_item->setData(QVariant::fromValue(image_path));
            image_item->setEditable(false);
            image_model->appendRow(image_item);
        }

        // 更新视频表
        auto video_model =
            qobject_cast<QStandardItemModel*>(ui->video_listView->model());
        video_model->clear();
        for (const auto& video_path : project->project_video_table) {
            auto video_item =
                new QStandardItem(QString::fromStdString(video_path));
            video_item->setData(QVariant::fromValue(video_path));
            video_item->setEditable(false);
            video_model->appendRow(video_item);
        }
    }
}

void ProjectManager::closeEvent(QCloseEvent* e) {
    for (auto& [name, project] : projects) {
        project->close();
    }
}
