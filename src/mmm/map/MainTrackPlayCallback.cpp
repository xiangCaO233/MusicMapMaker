#include <mmm/map/MainTrackPlayCallback.hpp>

// 播放完成完整一遍回调(传入是否循环)
void MainTrackPlayCallback::play_done(bool loop) const {}

// 帧基
void MainTrackPlayCallback::frameplaypos_updated(size_t frame_pos) {}

// 时间基
void MainTrackPlayCallback::timeplaypos_updated(
    std::chrono::nanoseconds time_pos) {}
