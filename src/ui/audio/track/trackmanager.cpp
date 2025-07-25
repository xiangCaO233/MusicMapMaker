#include "trackmanager.h"

#include <SDL3/SDL_audio.h>
#include <audio/control/audiocontroller.h>
#include <qlogging.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qpixmap.h>
#include <qtmetamacros.h>

#include <QCloseEvent>
#include <QDir>
#include <QStandardItemModel>
#include <ice/manage/AudioTrack.hpp>
#include <ice/out/play/sdl/SDLPlayer.hpp>
#include <memory>

#include "ui_trackmanager.h"

QStringList TrackManager::metaNames() {
    return {tr("title"),   tr("artist"), tr("album"),
            tr("bitrate"), tr("length"), tr("framecount")};
}

TrackManager::TrackManager(QWidget* parent)
    : HideableToolWindow(parent), ui(new Ui::TrackManager) {
    ui->setupUi(this);

    // 初始化sdl播放后端
    ice::SDLPlayer::init_backend();

    // 获取设备列表
    for (const auto& deviceinfo : ice::SDLPlayer::list_devices()) {
        ui->device_selection->addItem(QString::fromStdString(deviceinfo.name),
                                      deviceinfo.id);
    }

    // 选中默认设备
    // ui->device_selection->setCurrentIndex(0);

    // 初始化播放器
    player = std::make_shared<ice::SDLPlayer>();

    // 使用默认的设备打开
    player->open();
    // 更新选项
    ui->device_selection->setCurrentText(
        QString(SDL_GetAudioDeviceName(player->get_current_device())));

    // 初始化mixbux
    mixbus = std::make_shared<ice::MixBus>();

    // 设置输入为mixbus
    player->set_source(mixbus);

    // 直接开始播放
    player->start();

    // 音轨列表模型
    auto track_list_model = new QStandardItemModel(ui->track_list);
    ui->track_list->setModel(track_list_model);
    // 连接模型信号到项改变槽
    connect(track_list_model, &QStandardItemModel::itemChanged, this,
            &TrackManager::onItemChanged);

    // 音轨元数据表
    ui->metadata_table->setColumnCount(2);
    ui->metadata_table->setRowCount(metaNames().count());

    ui->metadata_table->setColumStretchs({1, 3});

    // 填充数据
    for (int row = 0; row < metaNames().count(); ++row) {
        auto nameitem = new QTableWidgetItem(metaNames()[row]);
        nameitem->setFlags(nameitem->flags() & ~Qt::ItemIsEditable);
        ui->metadata_table->setItem(row, 0, nameitem);

        auto valueitem = new QTableWidgetItem("");
        valueitem->setFlags(valueitem->flags() & ~Qt::ItemIsEditable);
        ui->metadata_table->setItem(row, 1, valueitem);
    }

    ui->album_info_widget->hide();
}

TrackManager::~TrackManager() {
    for (auto& controller : audio_controllers) {
        // 将音源节点移除混音器
        mixbus->remove_source(controller->output());

        // 释放已有的控制器
        controller->deleteLater();
    }
    // 清空 map
    audio_controllers.clear();

    // 确保在 player 停止后，所有对外的引用都断开
    if (player) {
        player->stop();
        player->close();
        if (player->joinable()) player->join();
    }

    // 在 quit_backend() 之前，手动销毁所有依赖 SDL 的 shared_ptr
    // 先销毁使用 player 的，再销毁 player 本身

    // 如果 AudioController 的析构依赖 mixbus 或 player, deleteLater() 已经处理
    // 先清理
    audio_tracks.clear();

    // 再清理MixBus
    if (mixbus) {
        mixbus->clear();
        mixbus.reset();
    }

    // 最后销毁 Player
    player.reset();

    // 现在，所有可能在析构时调用 SDL 函数的对象都已经被销毁了
    // 在这个时刻调用 quit_backend() 就是安全的
    ice::SDLPlayer::quit_backend();

    delete ui;
    qDebug() << "TrackManager deleted";
}

// 载入音频
void TrackManager::loadin_audio(const QString& audio_file) {
    auto track_it = audio_tracks.find(audio_file);
    if (track_it == audio_tracks.end()) {
        qDebug() << "lodin audio:" << audio_file;
        track_it = audio_tracks.insert(
            audio_file,
            audio_pool.get_or_load(threadpool, audio_file.toStdString()));
    }
    auto track = track_it.value();
    // 新建列表项
    auto track_item = new QStandardItem(QDir(audio_file).dirName());
    track_item->setEditable(false);
    track_item->setData(QVariant::fromValue(track));

    // 添加列表项
    auto model = qobject_cast<QStandardItemModel*>(ui->track_list->model());

    // 设置为可勾选
    track_item->setCheckable(true);
    // 默认未选中
    track_item->setCheckState(Qt::Unchecked);

    model->appendRow(track_item);
}
