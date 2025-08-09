#ifndef MMM_AUDIOLOADCALLBACK_HPP
#define MMM_AUDIOLOADCALLBACK_HPP

#include <ice/manage/AudioTrack.hpp>
#include <memory>
#include <string_view>

class AudioLoadCallback {
   public:
    // 析构AudioLoadCallback
    virtual ~AudioLoadCallback() = default;
    virtual std::weak_ptr<ice::AudioTrack> loadBack(
        std::string_view audio_path) = 0;
};

#endif  // MMM_AUDIOLOADCALLBACK_HPP
