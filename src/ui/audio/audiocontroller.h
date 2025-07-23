#ifndef AUDIOCONTROLLER_H
#define AUDIOCONTROLLER_H

#include <qobject.h>

#include <QWidget>

#include "template/HideableToolWindow.hpp"

namespace Ui {
class AudioController;
}

class AudioController : public HideableToolWindow {
    Q_OBJECT

   public:
    explicit AudioController(QWidget *parent = nullptr);
    ~AudioController();

   private:
    // 音频轨道对应的文件
    QString audio_track_file;

    Ui::AudioController *ui;
};

#endif  // AUDIOCONTROLLER_H
