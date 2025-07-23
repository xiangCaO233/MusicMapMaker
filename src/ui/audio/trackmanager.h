#ifndef TRACKMANAGER_H
#define TRACKMANAGER_H

#include <qcontainerfwd.h>
#include <qhash.h>
#include <qtmetamacros.h>

#include <QWidget>
#include <ice/manage/AudioPool.hpp>
#include <memory>

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
    ~TrackManager();

    // 载入音频
    void loadin_audio(const QString &audio_file);

   private slots:
    void on_add_track_button_clicked();

    void on_remove_track_button_clicked();

    void on_open_controller_button_clicked();

   private:
    // 各设备对应的播放器
    QHash<QString, ice::SDLPlayer> players;

    // 各个音轨对应的句柄
    QHash<QString, std::shared_ptr<ice::AudioTrack>> audio_tracks;

    // 各个音轨对应的控制器
    QHash<QString, AudioController *> audiocontrollers;

    // 线程池
    ice::ThreadPool threadpool{8};

    // 音频池
    ice::AudioPool audio_pool{ice::CodecBackend::FFMPEG};

    Ui::TrackManager *ui;
};

#endif  // TRACKMANAGER_H
