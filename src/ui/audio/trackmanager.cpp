#include "trackmanager.h"

#include <qlogging.h>
#include <qobject.h>
#include <qtmetamacros.h>

#include <QCloseEvent>
#include <QStandardItemModel>
#include <ice/out/play/sdl/SDLPlayer.hpp>

#include "ui_trackmanager.h"
#include "util/mutil.hpp"

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
    ui->device_selection->setCurrentIndex(0);

    auto track_list_model = new QStandardItemModel(ui->track_list);
    ui->track_list->setModel(track_list_model);
}

TrackManager::~TrackManager() {
    // 退出sdl后端
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
    model->appendRow(track_item);
}

void TrackManager::on_add_track_button_clicked() {
    auto audio_files =
        mutil::getOpenFiles(this, "导入音频",
                            {{tr("MP3 Music"), ".mp3"},
                             {tr("FLAC LosslessMusic"), ".flac"},
                             {tr("OGG PureAudio"), ".ogg"},
                             {tr("WAV WaveAudio"), ".wav"},
                             {tr("All Audio"), ".mp3 *.flac *.ogg *.wav"}});

    for (const auto& audio_file : audio_files) {
        qDebug() << "selecte:" << audio_file;
        loadin_audio(audio_file);
    }
}

void TrackManager::on_remove_track_button_clicked() {}

void TrackManager::on_open_controller_button_clicked() {}
