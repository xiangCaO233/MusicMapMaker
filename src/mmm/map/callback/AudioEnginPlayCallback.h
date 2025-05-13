#ifndef M_AUDIOENGIN_PLAYCALLBACK_H
#define M_AUDIOENGIN_PLAYCALLBACK_H

#include <qobject.h>
#include <qtmetamacros.h>

#include "../../../log/colorful-log.h"
#include "AudioManager.h"
#include "util/utils.h"

class AudioEnginPlayCallback : public QObject, public PlayposCallBack {
    Q_OBJECT
    double current_audio_time{0};
    int32_t count{195};
   signals:
    void music_play_callback(double time);

   public:
    std::atomic<bool> synclock{true};
    AudioEnginPlayCallback();
    ~AudioEnginPlayCallback();

    void playpos_call(double playpos) override;

    template <typename func>
    void waitfor_clear_buffer(func&& f) {
        // 执行任意传入lambda
        f(current_audio_time);
    }
};

#endif  // M_AUDIOENGIN_PLAYCALLBACK_H
