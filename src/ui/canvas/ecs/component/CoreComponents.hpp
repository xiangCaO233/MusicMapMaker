#ifndef MMM_CORECOMPONENTS_HPP
#define MMM_CORECOMPONENTS_HPP

#include <cstdint>
#include <glm/glm.hpp>
#include <string>

// 音乐时间属性
struct TimeComponent {
    uint32_t timestamp;  // ms
};

// 可视化属性
struct VisualsComponent {
    glm::vec4 color{1.f, 1.f, 1.f, 1.f};
    // 纹理资源路径
    std::string texture_path;
};

// 虚幻组件(即将添加)
struct GhostComponent {
    bool confirm{false};
};

// 删除标记组件(即将删除)
struct DeleteMarkComponent {
    bool confirm{false};
};

#endif  // MMM_CORECOMPONENTS_HPP
