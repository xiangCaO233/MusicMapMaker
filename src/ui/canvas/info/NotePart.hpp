#ifndef MMM_NOTEPART_HPP
#define MMM_NOTEPART_HPP

#include <entt.hpp>
#include <glm/glm.hpp>
#include <mmm/ObjectHandle.hpp>

// 悬浮信息
enum class NotePart {
    NONE,
    HEAD,
    HOLD_HEAD,
    HOLD_BODY,
    HOLD_END,
    SLIDE_HEAD,
    SLIDE_BODY,
    SLIDE_END,
};

// --- 用于打印 NotePart 枚举的辅助函数 ---
inline std::string to_string(NotePart part) {
    switch (part) {
        case NotePart::HEAD:
            return "HEAD";
        case NotePart::HOLD_HEAD:
            return "HOLD_HEAD";
        case NotePart::HOLD_BODY:
            return "HOLD_BODY";
        case NotePart::HOLD_END:
            return "HOLD_END";
        case NotePart::SLIDE_HEAD:
            return "SLIDE_HEAD";
        case NotePart::SLIDE_BODY:
            return "SLIDE_BODY";
        case NotePart::SLIDE_END:
            return "SLIDE_END";
        default:
            return "Unknown";
    }
}

struct MeshPartInfo {
    // 几何信息，直接从 Quad 获取
    glm::vec2 pos{0};
    glm::vec2 size{0};

    // 身份信息
    entt::entity source_entity{};
    NotePart part{NotePart::NONE};

    // 筛选信息
    int zIndex{0};

    // 物件句柄
    NoteHandle handle{};

    bool contains(const glm::vec2& p) const {
        // 计算矩形的右下角坐标
        const glm::vec2 bottom_right = pos + size;
        // 检查点的 x 和 y 坐标是否都在矩形范围内
        return p.x >= pos.x && p.x <= bottom_right.x && p.y >= pos.y &&
               p.y <= bottom_right.y;
    }
};

#endif  // MMM_NOTEPART_HPP
