#ifndef MMM_AUDIOLOADCALLBACK_HPP
#define MMM_AUDIOLOADCALLBACK_HPP

#include <chrono>
#include <ice/manage/AudioTrack.hpp>
#include <memory>
#include <string_view>

class AudioController;
class AudioLoadCallback {
   public:
    // 析构AudioLoadCallback
    virtual ~AudioLoadCallback() = default;
    virtual AudioController* getController(std::string_view audio_name) = 0;
    virtual void play_oneshot(std::string_view audio_name, float volume) = 0;
    virtual void set_playpos_for(std::string_view audio_name,
                                 std::chrono::nanoseconds time) = 0;
    virtual std::weak_ptr<ice::AudioTrack> loadBack(
        std::string_view audio_path, bool is_maintrack = false) = 0;
};

#endif  // MMM_AUDIOLOADCALLBACK_HPP
