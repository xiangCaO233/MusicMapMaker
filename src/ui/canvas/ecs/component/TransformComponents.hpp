#ifndef MMM_TRANSFORMCOMPONENTS_HPP
#define MMM_TRANSFORMCOMPONENTS_HPP

#include <entt.hpp>
#include <glm/glm.hpp>
#include <render/texture/TexMode.hpp>
#include <render/texture/TextureInfo.hpp>
#include <vector>

// 转换组件1-指示此物件在画布中的逻辑中心位置
struct TransformComponent {
    float y;
};

struct GeneratedMesh {
    // 标记这个网格属于哪个实体
    entt::entity source_entity;
    struct Quad {
        glm::vec2 pos;
        glm::vec2 size;
        // 纹理信息
        TextureInfo texture{};
        // 绘制层级，值越小越先绘制
        int zIndex{0};
        bool glow{false};
        TexScaleMode mode{TexScaleMode::FORCE_FILL};
    };
    std::vector<Quad> mesh;
};

#endif  // MMM_TRANSFORMCOMPONENTS_HPP
