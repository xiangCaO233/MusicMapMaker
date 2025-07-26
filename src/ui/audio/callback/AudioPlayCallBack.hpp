#ifndef MMM_AUDIOPLAYCALLBACK_HPP
#define MMM_AUDIOPLAYCALLBACK_HPP

#include <audio/control/audiocontroller.h>
#include <qobject.h>

#include <ice/core/PlayCallBack.hpp>

class AudioController;
class PlayPosCallBack : public QObject, public ice::PlayCallBack {
    Q_OBJECT

   public:
    explicit PlayPosCallBack(AudioController* controller);

   signals:
    void playDone() const;
    void update_framepos();
    void update_timepos();

   private:
    AudioController* refcontroller;

    // 实现回调
    void play_done(bool isloop) const override;
    void frameplaypos_updated(size_t frame_pos) override;
    void timeplaypos_updated(std::chrono::nanoseconds time_pos) override;
};

#endif  // MMM_AUDIOPLAYCALLBACK_HPP
