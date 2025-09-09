#ifndef MMM_EFFECTCOMPONENTS_HPP
#define MMM_EFFECTCOMPONENTS_HPP

#include <chrono>
#include <glm/glm.hpp>
#include <map/MapCanvas.hpp>
#include <mmm/obj/Note.hpp>

// --- 特效状态组件 (每个轨道一个) ---
struct EffectComponent {
    // 特效轨道号 (用于反向查找)
    int track;

    // --- 视觉状态 ---
    // 当前应该播放哪种纹理序列
    EffectTextureType texture_type;

    // 特效还需持续播放的时间
    double duration{.0};

    // 特效的帧进度索引
    int32_t frame_index{-1};
};

// --- 音效状态 ---
struct SoundStateComponent {
    // 使用 map 来存储每种音效类型对应的待播放次数（强度）
    // key: SoundEffectType
    // value: uint32_t (count)
    std::map<SoundEffectType, uint32_t> pending_sounds;
};

// 轨道标签，便于查找
struct TrackIdentifierComponent {
    uint32_t track;
};

#endif  // MMM_EFFECTCOMPONENTS_HPP
