#ifndef MMM_NOTEPART_HPP
#define MMM_NOTEPART_HPP

#include <entt.hpp>
#include <glm/glm.hpp>

// 悬浮信息
enum class NotePart {
    HEAD,
    HOLD_BODY,
    SLIDE_BODY,
    HOLD_END,
    SLIDE_END,
};

struct MeshPartInfo {
    // 几何信息，直接从 Quad 获取
    glm::vec2 pos;
    glm::vec2 size;

    // 身份信息
    entt::entity source_entity;
    NotePart part;

    // 筛选信息
    int zIndex;
};

#endif  // MMM_NOTEPART_HPP
