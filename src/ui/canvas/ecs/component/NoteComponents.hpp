#ifndef MMM_NOTECOMPONENTS_HPP
#define MMM_NOTECOMPONENTS_HPP

#include <cstdint>
#include <mmm/ObjectHandle.hpp>

// 标记这是一个真实Note，并存储其轨道信息和uuid
struct NoteComponent {
    uint32_t track_index;
    // InvalidNoteUUID标记这是一个即将创建的Note，并存储其轨道信息/无uuid
    NoteUUID sourceUUID{InvalidNoteUUID};
};

// 标记这是一个即将创建的Note(用于同步系统筛选和更新)
struct CreatingNoteComponent {};

// Hold Note的特有组件
struct HoldComponent {
    // ms
    uint32_t duration;
};

// Flick Note的特有组件
struct FlickComponent {
    // 轨道偏移量, e.g., +1 or -1
    int64_t delta_track;
};

#endif  // MMM_NOTECOMPONENTS_HPP
