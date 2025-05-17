#include "projectcontroller.h"

#include <AudioManager.h>
#include <qlogging.h>
#include <qnamespace.h>
#include <qsharedpointer.h>
#include <qtmetamacros.h>

#include <QDesktopServices>
#include <QFileDialog>
#include <QMenu>
#include <QModelIndex>
#include <QStandardItemModel>
#include <QUrl>
#include <algorithm>
#include <filesystem>
#include <memory>

#include "../../mmm/MapWorkProject.h"
#include "../../util/mutil.h"
#include "audio/BackgroundAudio.h"
#include "colorful-log.h"
#include "guide/newmapguide.h"
#include "mmm/map/MMap.h"
#include "ui_mprojectcontroller.h"

MProjectController::MProjectController(QWidget* parent)
    : QWidget(parent), ui(new Ui::MProjectController) {
    ui->setupUi(this);

    // 初始化listview模型
    QStandardItemModel* map_list_model =
        new QStandardItemModel(ui->map_list_view);
    QStandardItemModel* audio_list_model =
        new QStandardItemModel(ui->audio_list_view);
    QStandardItemModel* image_list_model =
        new QStandardItemModel(ui->image_list_view);
    QStandardItemModel* video_list_model =
        new QStandardItemModel(ui->video_list_view);

    // 应用并设置不可编辑
    ui->map_list_view->setModel(map_list_model);
    ui->map_list_view->setEditTriggers(QAbstractItemView::NoEditTriggers);

    ui->audio_list_view->setModel(audio_list_model);
    ui->audio_list_view->setEditTriggers(QAbstractItemView::NoEditTriggers);

    ui->image_list_view->setModel(image_list_model);
    ui->image_list_view->setEditTriggers(QAbstractItemView::NoEditTriggers);

    ui->video_list_view->setModel(video_list_model);
    ui->video_list_view->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // 默认无选择
    ui->project_selector->addItem(QString(tr("No Project")));
}

MProjectController::~MProjectController() { delete ui; }

// 新项目槽函数
void MProjectController::new_project(std::shared_ptr<MapWorkProject>& project) {
    // 添加到项目列表
    project_mapping[project->config.project_name] = project;

    // 添加选中项
    ui->project_selector->addItem(
        QString::fromStdString(project->config.project_name),
        QVariant::fromValue(project));

    // 立即选中此项目
    ui->project_selector->setCurrentIndex(ui->project_selector->count() - 1);

    // 检查关闭项目按钮是否启用
    if (!ui->close_project_button->isEnabled()) {
        ui->close_project_button->setEnabled(true);
    }
}

// 选择项目事件
void MProjectController::on_project_selector_currentIndexChanged(int index) {
    // 获取当前选中项的数据
    QVariant var = ui->project_selector->currentData();
    auto project = var.value<std::shared_ptr<MapWorkProject>>();
    if (project) {
        select_project(project);
    }
}

// 选择项目
void MProjectController::select_project(
    std::shared_ptr<MapWorkProject>& project) {
    auto map_model =
        qobject_cast<QStandardItemModel*>(ui->map_list_view->model());
    map_model->clear();
    auto audio_model =
        qobject_cast<QStandardItemModel*>(ui->audio_list_view->model());
    audio_model->clear();
    auto image_model =
        qobject_cast<QStandardItemModel*>(ui->image_list_view->model());
    image_model->clear();
    auto video_model =
        qobject_cast<QStandardItemModel*>(ui->video_list_view->model());
    video_model->clear();
    if (project) {
        // 初始化每个tabwidget显示内容
        // maps
        for (const auto& map : project->maps) {
            QStandardItem* map_item =
                new QStandardItem(QString::fromStdString(map->map_name));
            map_item->setData(QVariant::fromValue(map), Qt::UserRole + 1);
            map_model->appendRow(map_item);
        }

        // audios
        for (const auto& audio_path : project->audio_paths) {
            std::filesystem::path p(audio_path);
            QStandardItem* audio_path_item = new QStandardItem(
                QString::fromStdString(p.filename().string()));
            audio_path_item->setData(QVariant::fromValue(audio_path),
                                     Qt::UserRole + 1);
            audio_model->appendRow(audio_path_item);
        }

        // images
        for (const auto& image_path : project->image_paths) {
            std::filesystem::path p(image_path);
            QStandardItem* image_path_item = new QStandardItem(
                QString::fromStdString(p.filename().string()));
            image_path_item->setData(QVariant::fromValue(image_path),
                                     Qt::UserRole + 1);
            image_model->appendRow(image_path_item);
        }

        // videos
        for (const auto& video_path : project->video_paths) {
            std::filesystem::path p(video_path);
            QStandardItem* video_path_item = new QStandardItem(
                QString::fromStdString(p.filename().string()));
            video_path_item->setData(QVariant::fromValue(video_path),
                                     Qt::UserRole + 1);
            video_model->appendRow(video_path_item);
        }
    }

    if (ui->audio_device_selector->count() == 0) {
        // 初始化设备列表
        ui->audio_device_selector->clear();
        // 更新设备列表
        for (const auto& [device_id, device] : (*BackgroundAudio::devices())) {
            ui->audio_device_selector->addItem(
                QString::fromStdString(device->device_name));
        }
    }
    // 选中此项目选中的设备
    if (project) {
        if (selected_project != project) {
            selected_project = project;
        } else {
            device_sync_lock = true;
        }
        // 选中此设备
        ui->audio_device_selector->setCurrentText(
            QString::fromStdString(project->devicename));
    }
}

// 谱面列表双击事件
void MProjectController::on_map_list_view_doubleClicked(
    const QModelIndex& index) {
    auto map_model =
        qobject_cast<QStandardItemModel*>(ui->map_list_view->model());
    auto map = map_model->itemFromIndex(index)
                   ->data(Qt::UserRole + 1)
                   .value<std::shared_ptr<MMap>>();

    // 发送选择map信号
    emit select_map(map);
}

// 音频列表双击事件
void MProjectController::on_audio_list_view_doubleClicked(
    const QModelIndex& index) {}

// 图片列表双击事件
void MProjectController::on_image_list_view_doubleClicked(
    const QModelIndex& index) {}

// 视频列表双击事件
void MProjectController::on_video_list_view_doubleClicked(
    const QModelIndex& index) {}

// 谱面列表上下文菜单事件
void MProjectController::on_map_list_view_customContextMenuRequested(
    const QPoint& pos) {
    // 使用当前选中项而不是点击位置
    QModelIndex index = ui->map_list_view->currentIndex();

    // 生成菜单
    QMenu menu;

    // 根据点击位置添加菜单项
    if (!index.isValid()) {
    } else {
        // 获得选中的map
        auto selected_map =
            qobject_cast<QStandardItemModel*>(ui->map_list_view->model())
                ->itemFromIndex(index)
                ->data()
                .value<std::shared_ptr<MMap>>();
        // 在文件管理器中打开
        menu.addAction(tr("Open In FileBrowser"),
                       [
                           // 复制捕获防止智能指针引用丢失
                           = ]() {
                           // qDebug() << selected_map->map_name;
                           // 获取文件上一级路径
                           QDir dir(selected_map->map_file_path.parent_path());
                           // 转换为本地文件URL
                           QUrl url = QUrl::fromLocalFile(dir.absolutePath());
                           // 使用文件管理器打开
                           QDesktopServices::openUrl(url);
                       });
    }
    // 添加通用菜单项
    menu.addSeparator();
    menu.addAction(tr("Import Map"), [&]() {
        auto options = QFileDialog::DontUseNativeDialog;
        auto fileNames = QFileDialog::getOpenFileNames(
            this, tr("选择谱面"), XLogger::last_select_directory,
            tr("谱面文件(*.osu *.imd *.mc)"), nullptr, options);

        // TODO(xiang 2025-04-17): 实现项目中导入谱面
        for (auto& name : fileNames) {
            qDebug() << "selected:" << name;
        }
    });
    menu.addAction(tr("Create New Map"), [&]() {
        // 调用创建谱面函数
        if (!selected_project) {
            XERROR("请先打开项目");
        } else {
            qDebug() << "创建谱面";
            // 显示模态对话框
            NewMapGuide dialog;
            if (dialog.exec() == QDialog::Accepted) {
                // 用户点击了OK
                XINFO("确认创建map");
                // 检查创建所需参数
                // 音频路径
                auto audio_file_path = dialog.music_path;
                BackgroundAudio::unload_audio(audio_file_path);

                // 构造map
                auto map = std::make_shared<MMap>();
                std::filesystem::path music_path(audio_file_path);
                // 不存在则拷贝过去
                if (!mutil::fileExistsInPath(music_path,
                                             selected_project->ppath)) {
                    // 拷贝并更新音频路径
                    mutil::copyFileToPath(music_path, selected_project->ppath,
                                          map->audio_file_abs_path);
                    selected_project->audio_paths.emplace_back(
                        QDir(map->audio_file_abs_path)
                            .absolutePath()
                            .toStdString());
                } else {
                    // 直接使用音频路径
                    map->audio_file_abs_path = music_path;
                }

                auto audio_path_str = map->audio_file_abs_path.generic_string();
                if (BackgroundAudio::loadin_audio(audio_path_str) ==
                    -1) {
                    XERROR("无法加载音频");
                    return;
                }
                // 谱面时长
                // XINFO(QString("map_length:[%1]")
                //           .arg(dialog.map_length)
                //           .toStdString());
                map->map_length = dialog.map_length;

                // pbpm
                // XINFO(QString("pbpm:[%1]").arg(dialog.pbpm).toStdString());
                map->preference_bpm = dialog.pbpm;

                // author
                // XINFO(QString("author:[%1]").arg(dialog.author).toStdString());
                map->author = dialog.author;

                // bgpath
                // XINFO(QString("bg:[%1]").arg(dialog.bg_path).toStdString());
                map->bg_path = dialog.bg_path;
                std::filesystem::path bg_path(dialog.bg_path);
                // 不存在则拷贝过去
                if (!mutil::fileExistsInPath(bg_path,
                                             selected_project->ppath)) {
                    // 拷贝并更新图片路径
                    mutil::copyFileToPath(bg_path, selected_project->ppath,
                                          map->bg_path);
                    selected_project->image_paths.emplace_back(
                        map->bg_path.generic_string());
                } else {
                    // 直接使用图片路径
                    map->bg_path = dialog.bg_path;
                }

                // 标题
                // XINFO(QString("title:[%1]").arg(dialog.title).toStdString());
                // XINFO(QString("title unicode:[%1]")
                //           .arg(dialog.title_unicode)
                //           .toStdString());
                // 艺术家
                // XINFO(QString("artist:[%1]").arg(dialog.artist).toStdString());
                // XINFO(QString("artist unicode:[%1]")
                //           .arg(dialog.artist_unicode)
                //           .toStdString());
                map->title = dialog.title;
                map->title_unicode = dialog.title_unicode;
                map->artist = dialog.artist;
                map->artist_unicode = dialog.artist_unicode;

                // 键数
                map->orbits = dialog.orbits;

                // 版本
                map->version = dialog.version;

                // 生成谱面名称
                map->map_name =
                    "[" + std::to_string(map->orbits) + "k] " + map->version;
                // 谱面路径使用相对项目路径
                map->map_file_path =
                    selected_project->ppath / (map->map_name + ".mmm");

                // 添加map
                selected_project->add_new_map(map);

                // 触发更新
                select_project(selected_project);

                // 资源不存在则拷贝到当前项目
            } else {
                // 用户取消或关闭了对话框
                XWARN("取消创建map");
            }
        }
    });

    // 显示菜单
    menu.exec(ui->map_list_view->viewport()->mapToGlobal(pos));
}

// 音频列表上下文菜单事件
void MProjectController::on_audio_list_view_customContextMenuRequested(
    const QPoint& pos) {}

// 图片列表上下文菜单事件
void MProjectController::on_image_list_view_customContextMenuRequested(
    const QPoint& pos) {}

// 视频列表上下文菜单事件
void MProjectController::on_video_list_view_customContextMenuRequested(
    const QPoint& pos) {}

// 关闭项目事件
void MProjectController::on_close_project_button_clicked() {
    auto current_select_text = ui->project_selector->currentText();
    if (current_select_text != "No Project") {
        // 获取选中的项目指针
        QVariant var = ui->project_selector->currentData();
        auto project = var.value<std::shared_ptr<MapWorkProject>>();
        auto project_it = project_mapping.find(project->config.project_name);
        if (project_it != project_mapping.end()) {
            // 移除当前选中项
            ui->project_selector->removeItem(
                ui->project_selector->currentIndex());
            // 从项目列表移除
            project_mapping.erase(project_it);
            // 检查当前是否还有其他项目
            if (!project_mapping.empty()) {
                // 选中另一个项目
                on_project_selector_currentIndexChanged(1);
            } else {
                // 选中空白并禁用关闭项目按钮
                ui->project_selector->setCurrentIndex(0);
                std::shared_ptr<MapWorkProject> p = nullptr;
                select_project(p);
                ui->close_project_button->setEnabled(false);
            }
        }
    }
}

// 选择音频输出设备事件
void MProjectController::on_audio_device_selector_currentTextChanged(
    const QString& arg1) {
    if (device_sync_lock) {
        device_sync_lock = false;
        return;
    }

    if (selected_project) {
        // 获取并设置项目设备为当前选中的音频设备
        auto device = ui->audio_device_selector->currentText().toStdString();
        selected_project->set_audio_device(device);

        BackgroundAudio::init_device(selected_project->devicename);
        // 更新音量
        BackgroundAudio::set_global_volume(selected_project->devicename,
                                           BackgroundAudio::global_volume);
        BackgroundAudio::set_music_volume(selected_project->devicename,
                                          BackgroundAudio::music_orbits_volume);
        BackgroundAudio::set_effects_volume(
            selected_project->devicename,
            BackgroundAudio::effect_orbits_volume);

        // if (BackgroundAudio::enable_pitch_alt) {
        //     BackgroundAudio::set_play_speed(selected_project->devicename,
        //                                     BackgroundAudio::global_speed,
        //                                     true);
        // } else {
        //     BackgroundAudio::set_play_speed(selected_project->devicename,
        //                                     BackgroundAudio::orbit_speed);
        // }
    }
}
