#include <audio/track/trackmanager.h>
#include <project/projectconfig.h>
#include <project/projectmanager.h>
#include <qlogging.h>
#include <ui_projectmanager.h>

#include <QCloseEvent>
#include <QStandardItemModel>
#include <canvas/map/MapCanvas.hpp>
#include <mmm/project/MProject.hpp>

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
    delete ui;
    qDebug() << "ProjectManager deleted";
}

// 初始化管理器
void ProjectManager::initService(MapCanvas* canvas,
                                 TrackManager* trackmanager) {
    service = new ProjectService(canvas, trackmanager, this);
    connect(this, &ProjectManager::openProject, service,
            &ProjectService::onOpenProject);
    connect(this, &ProjectManager::closeProject, service,
            &ProjectService::onCloseProject);
    connect(service, &ProjectService::updateProjectListView, this,
            &ProjectManager::onUpdateProjectListView);
    connect(service, &ProjectService::activateProject, this,
            &ProjectManager::onActivateProject);
}

void ProjectManager::onUpdateProjectListView(
    const std::unordered_map<std::string, std::unique_ptr<MProject>, StringHash,
                             std::equal_to<>>* projects) const {
    auto model = qobject_cast<QStandardItemModel*>(ui->project_list->model());
    model->clear();
    for (const auto& [name, project] : *projects) {
        // 新建列表项
        auto project_item = new QStandardItem(QString::fromStdString(name));
        project_item->setData(QVariant::fromValue(name));
        project_item->setEditable(false);

        // 添加列表项
        model->appendRow(project_item);
    }
}

// 激活项目
void ProjectManager::onActivateProject(MProject* activated_project) {
    if (activated_project) {
        // 更新谱面表
        auto map_model =
            qobject_cast<QStandardItemModel*>(ui->map_listView->model());
        map_model->clear();

        for (const auto& [map_name, map] :
             activated_project->project_maps_table) {
            auto map_item = new QStandardItem(QString::fromStdString(map_name));
            map_item->setData(QVariant::fromValue(map.get()));
            map_item->setEditable(false);
            map_model->appendRow(map_item);
        }

        // 更新音频表
        auto audio_model =
            qobject_cast<QStandardItemModel*>(ui->audio_listView->model());
        audio_model->clear();
        for (const auto& [audio_name, track] :
             activated_project->project_audios_table) {
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
        for (const auto& image_path : activated_project->project_image_table) {
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
        for (const auto& video_path : activated_project->project_video_table) {
            auto video_item =
                new QStandardItem(QString::fromStdString(video_path));
            video_item->setData(QVariant::fromValue(video_path));
            video_item->setEditable(false);
            video_model->appendRow(video_item);
        }
        // 绑定配置ui的配置内容
        config_ui->bind_config(&activated_project->project_config);
    }
}

void ProjectManager::onMapCanvasThreadStopped() {
    // 在画布的线程完全停止后才释放项目资源
    qDebug() << "ProjectManager: delete service";
    delete service;
}

void ProjectManager::closeEvent(QCloseEvent* e) {
    service->selectMap("", nullptr);
    qDebug() << "ProjectManager: delete configui";
    delete config_ui;
}
