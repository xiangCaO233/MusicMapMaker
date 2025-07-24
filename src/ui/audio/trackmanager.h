#ifndef TRACKMANAGER_H
#define TRACKMANAGER_H

#include <qcontainerfwd.h>
#include <qhash.h>
#include <qstandarditemmodel.h>
#include <qtmetamacros.h>

#include <QWidget>
#include <ice/manage/AudioPool.hpp>
#include <memory>

#include "ice/core/IAudioNode.hpp"
#include "ice/core/MixBus.hpp"
#include "ice/manage/AudioTrack.hpp"
#include "ice/out/play/sdl/SDLPlayer.hpp"
#include "ice/thread/ThreadPool.hpp"
#include "template/HideableToolWindow.hpp"

namespace Ui {
class TrackManager;
}

class AudioController;

class TrackManager : public HideableToolWindow {
    Q_OBJECT

   public:
    explicit TrackManager(QWidget *parent = nullptr);
    ~TrackManager() override;

    // 载入音频
    void loadin_audio(const QString &audio_file);

   private slots:
    void on_add_track_button_clicked();

    void on_remove_track_button_clicked();

    void on_open_controller_button_clicked();

    void on_track_list_clicked(const QModelIndex &index);

    void on_track_list_doubleClicked(const QModelIndex &index);

    void onItemChanged(QStandardItem *item);

    // 音频控制信号
    // 收到控制器被关闭的信号
    void onControllerHide(HideableToolWindow *wptr) const;
    void onControllerOutputUpdate(const AudioController *controller,
                                  std::shared_ptr<ice::IAudioNode> oldnode,
                                  std::shared_ptr<ice::IAudioNode> newnode);

   private:
    static QStringList metaNames();
    // 各设备对应的播放器
    QHash<QString, ice::SDLPlayer> players;

    // 各个音轨对应的句柄
    QHash<QString, std::shared_ptr<ice::AudioTrack>> audio_tracks;

    // 各个音轨对应的控制器
    QHash<QString, AudioController *> audio_controllers;

    // 线程池
    ice::ThreadPool threadpool{8};

    // 音频池
    ice::AudioPool audio_pool{ice::CodecBackend::FFMPEG};

    // 播放器实例
    std::shared_ptr<ice::SDLPlayer> player;

    // 混音器
    std::shared_ptr<ice::MixBus> mixbus{nullptr};

    // 创建轨道控制器
    decltype(audio_controllers.begin()) makeController(
        const std::shared_ptr<ice::AudioTrack> &track, QStandardItem *refitem);

    Ui::TrackManager *ui;
};

#endif  // TRACKMANAGER_H
