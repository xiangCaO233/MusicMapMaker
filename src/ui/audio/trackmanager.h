#ifndef TRACKMANAGER_H
#define TRACKMANAGER_H

#include <qcontainerfwd.h>
#include <qhash.h>
#include <qtmetamacros.h>

#include <QWidget>
#include <ice/manage/AudioPool.hpp>

namespace Ui {
class TrackManager;
}

class AudioController;

class TrackManager : public QWidget {
    Q_OBJECT

   public:
    explicit TrackManager(QWidget *parent = nullptr);
    ~TrackManager();

   signals:
    void close_signal();

   protected:
    void closeEvent(QCloseEvent *event) override;

   private:
    // 各个音轨对应的控制器
    QHash<QString, AudioController *> audiocontrollers;

    ice::AudioPool audio_pool;

    Ui::TrackManager *ui;
};

#endif  // TRACKMANAGER_H
