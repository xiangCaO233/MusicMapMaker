#ifndef MMM_NOTECOMPONENTS_HPP
#define MMM_NOTECOMPONENTS_HPP

#include <cstdint>

// 标记这是一个Note，并存储其轨道信息
struct NoteComponent {
    int32_t track_index;
};

// Hold Note的特有属性
struct HoldComponent {
    // ms
    int32_t duration;
};

// Flick Note的特有属性
struct FlickComponent {
    // 轨道偏移量, e.g., +1 or -1
    int32_t delta_track;
};

#endif  // MMM_NOTECOMPONENTS_HPP
