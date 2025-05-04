#include "BackgroundAudio.h"

// 音频管理器
std::shared_ptr<XAudioManager> BackgroundAudio::audiomanager;

double BackgroundAudio::audio_buffer_offset{
    double(-xutil::plannerpcmpos2milliseconds(
        x::Config::mix_buffer_size / 3.0,
        static_cast<int>(x::Config::samplerate)))};

// 变调是否启用
bool BackgroundAudio::enable_pitch_alt{false};

// 轨道音量
float BackgroundAudio::music_orbits_volume{1.0f};
float BackgroundAudio::effect_orbits_volume{1.0f};

// 全局音量
float BackgroundAudio::global_volume{0.5};

// 轨道速度
float BackgroundAudio::orbit_speed{1.0f};

// 全局速度
float BackgroundAudio::global_speed{1.0f};
