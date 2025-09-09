#ifndef MMM_EFFECTCOMPONENTS_HPP
#define MMM_EFFECTCOMPONENTS_HPP

#include <chrono>
#include <glm/glm.hpp>
#include <map/MapCanvas.hpp>
#include <mmm/obj/Note.hpp>

// --- 标签/请求组件 (瞬态) ---

// 当一个Note在本帧越过判定线时，由SyncSystem添加
struct PlayEffectRequest {
    // 可以带一些参数，比如特效类型，触发位置等
    NoteType triggered_by_note_type;
    float trigger_x_pos;  // 特效应该在哪个X坐标播放
    // ... 其他需要的参数 ...
};

// --- 特效实体组件 ---

// 标记这是一个正在播放的特效实体
struct EffectComponent {
    std::chrono::steady_clock::time_point start_time;  // 特效开始播放的现实时间
    float duration_ms;                                 // 特效总持续时间 (ms)
    int frame_count;                                   // 特效的总帧数
    std::string
        texture_dir;  // 特效纹理所在的目录 (e.g., "nomal_hit_effect_dir")

    // 构造函数
    EffectComponent(float dur, int frames, std::string_view dir)
        : start_time(std::chrono::steady_clock::now()),
          duration_ms(dur),
          frame_count(frames),
          texture_dir(dir) {}
};

// 特效的单独变换组件
struct EffectTransformComponent {
    // 在屏幕上的绝对位置
    glm::vec2 position;
    glm::vec2 size;
};

// --- 音效请求组件 (瞬态) ---
struct PlaySoundRequest {
    SoundEffectType sound_type;
};
#endif  // MMM_EFFECTCOMPONENTS_HPP
