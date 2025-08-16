#ifndef MMM_NOTECOMPONENTS_HPP
#define MMM_NOTECOMPONENTS_HPP

#include <cstdint>
#include <glm/glm.hpp>
#include <mmm/NoteHandle.hpp>
#include <vector>

// 标记这是一个Note，并存储其轨道信息
struct NoteComponent {
    uint32_t timestamp;
    uint32_t track_index;
    NoteHandle sourceHandle;
};

// 转换组件1-指示此物件在画布中的逻辑中心位置
struct TransformComponent_1 {
    float x;
    float y;
};

// 转换组件2-指示此物件的完整绘制网格列表
struct TransformComponent_2 {
    struct Quad {
        glm::vec2 pos;
        glm::vec2 size;
    };
    std::vector<Quad> mesh;
};

// Hold Note的特有属性
struct HoldComponent {
    // ms
    uint32_t duration;
};

// Flick Note的特有属性
struct FlickComponent {
    // 轨道偏移量, e.g., +1 or -1
    uint32_t delta_track;
};

#endif  // MMM_NOTECOMPONENTS_HPP
