#ifndef MMM_CORECOMPONENTS_HPP
#define MMM_CORECOMPONENTS_HPP

#include <cstdint>
#include <glm/glm.hpp>
#include <string>

// 音乐时间属性
struct MusicTimeComponent {
    int32_t timestamp;  // ms
};

// 屏幕空间变换属性 (由System计算生成)
struct TransformComponent {
    glm::vec2 position;
    glm::vec2 size;
    float rotation{0.0f};
};

// 可视化属性
struct VisualsComponent {
    glm::vec4 color{1.f, 1.f, 1.f, 1.f};
    // 纹理资源路径
    std::string texture_path;
};

#endif  // MMM_CORECOMPONENTS_HPP
