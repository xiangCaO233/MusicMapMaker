#ifndef MMM_MAINTRACKPLAYCALLBACK_HPP
#define MMM_MAINTRACKPLAYCALLBACK_HPP

#include <ice/core/PlayCallBack.hpp>

class MainTrackPlayCallback : public ice::PlayCallBack {
   public:
    MainTrackPlayCallback();
    // 析构MainTrackPlayCallback
    ~MainTrackPlayCallback() override = default;

    // 播放完成完整一遍回调(传入是否循环)
    void play_done(bool loop) const override;

    // 帧基
    void frameplaypos_updated(size_t frame_pos) override;

    // 时间基
    void timeplaypos_updated(std::chrono::nanoseconds time_pos) override;
};
#endif  // MMM_MAINTRACKPLAYCALLBACK_HPP
