#include <audio/callback/AudioPlayCallBack.hpp>

PlayPosCallBack::PlayPosCallBack(AudioController* controller)
    : QObject(controller), refcontroller(controller) {}

// 实现回调
void PlayPosCallBack::frameplaypos_updated(size_t frame_pos) {
    refcontroller->set_uiframe_pos(frame_pos);
    emit update_framepos();
}

void PlayPosCallBack::timeplaypos_updated(std::chrono::nanoseconds time_pos) {
    refcontroller->set_uitime_pos(time_pos);
    emit update_timepos();
}
