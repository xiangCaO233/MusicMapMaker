#ifndef TRACKMANAGER_H
#define TRACKMANAGER_H

#include <qcontainerfwd.h>
#include <qhash.h>
#include <qstandarditemmodel.h>
#include <qtmetamacros.h>

#include <GLCanvas.hpp>
#include <QWidget>
#include <audio/track/SourceNodePool.hpp>
#include <ice/core/IAudioNode.hpp>
#include <ice/core/MixBus.hpp>
#include <ice/manage/AudioPool.hpp>
#include <ice/manage/AudioTrack.hpp>
#include <ice/out/play/sdl/SDLPlayer.hpp>
#include <ice/thread/ThreadPool.hpp>
#include <memory>
#include <mmm/project/AudioLoadCallback.hpp>
#include <template/HideableToolWindow.hpp>

namespace Ui {
class TrackManager;
}

class AudioController;

class TrackManager : public HideableToolWindow, public AudioLoadCallback {
    Q_OBJECT

   public:
    explicit TrackManager(QWidget *parent = nullptr);
    ~TrackManager() override;

    // 载入音频
    std::weak_ptr<ice::AudioTrack> loadin_audio(const QString &audio_file,
                                                bool is_maintrack);

    // 获取音频轨道
    std::weak_ptr<ice::AudioTrack> get_track(const QString &audio_name);

    // 获取音频控制器
    AudioController *get_controller(const QString &audio_name);

    // 获取所有主音轨名
    const QStringList &get_maintrack() const;

    // 设置主音轨名
    void set_maintrack(const QString &name);

    // 回调实现
    std::weak_ptr<ice::AudioTrack> loadBack(std::string_view audio_path,
                                            bool is_maintrack = false) override;
    AudioController *getController(std::string_view audio_name) override;

    void set_playpos_for(std::string_view audio_name,
                         std::chrono::nanoseconds time) override;

    void play_oneshot(std::string_view audio_name, float volume) override;

   signals:
    void audioLoadcbk_initialized(AudioLoadCallback *cbk);

   private slots:
    void on_add_track_button_clicked();

    void on_remove_track_button_clicked();

    void on_open_controller_button_clicked();

    void on_track_list_clicked(const QModelIndex &index);

    void on_track_list_doubleClicked(const QModelIndex &index);

    void onItemChanged(QStandardItem *item);

   protected:
    void closeEvent(QCloseEvent *event) override;

   private:
    static QStringList metaNames();

    // 各设备对应的播放器
    QHash<QString, ice::SDLPlayer> players;

    // 各个音轨对应的句柄
    QHash<QString, std::shared_ptr<ice::AudioTrack>> audio_tracks;

    // 各个音轨对应的控制器
    QHash<QString, AudioController *> audio_controllers;

    // 主音轨
    QStringList maintrack_names;

    // 线程池
    ice::ThreadPool threadpool{2};

    // 音频池
    ice::AudioPool audio_pool{ice::CodecBackend::FFMPEG};

    // 播放器实例
    std::shared_ptr<ice::SDLPlayer> player;

    // 混音器
    std::shared_ptr<ice::MixBus> mixbus{nullptr};

    // 为每个音轨维护的SourceNode池
    QHash<QString, std::shared_ptr<SourceNodePool>> one_shot_pool;

    // 创建轨道控制器
    decltype(audio_controllers.begin()) makeController(
        const std::shared_ptr<ice::AudioTrack> &track, QStandardItem *refitem);

    Ui::TrackManager *ui;
};

#endif  // TRACKMANAGER_H
