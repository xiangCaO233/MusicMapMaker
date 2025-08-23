#ifndef MMM_NOTECOMPONENTS_HPP
#define MMM_NOTECOMPONENTS_HPP

#include <cstdint>
#include <mmm/ObjectHandle.hpp>

// 标记这是一个Note，并存储其轨道信息
struct NoteComponent {
    uint32_t track_index;
    NoteHandle sourceHandle;
};

// Hold Note的特有组件
struct HoldComponent {
    // ms
    uint32_t duration;
};

// Flick Note的特有组件
struct FlickComponent {
    // 轨道偏移量, e.g., +1 or -1
    uint32_t delta_track;
};

#endif  // MMM_NOTECOMPONENTS_HPP
