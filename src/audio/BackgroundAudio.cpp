#include "BackgroundAudio.h"

// 音频管理器
std::shared_ptr<XAudioManager> BackgroundAudio::audiomanager;

// 轨道音量
float BackgroundAudio::orbit_volume{1.0f};

// 全局音量
float BackgroundAudio::global_volume{0.5};

// 轨道速度
float BackgroundAudio::orbit_speed{1.0f};

// 全局速度
float BackgroundAudio::global_speed{1.0f};
