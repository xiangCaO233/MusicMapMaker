#ifndef MMM_TRANSFORMCOMPONENTS_HPP
#define MMM_TRANSFORMCOMPONENTS_HPP

#include <glm/glm.hpp>
#include <render/texture/TextureInfo.hpp>
#include <vector>

// 转换组件1-指示此物件在画布中的逻辑中心位置
struct TransformComponent_1 {
    float y;
};

// 转换组件2-指示此实体的完整绘制网格列表
struct TransformComponent_2 {
    struct Quad {
        glm::vec2 pos;
        glm::vec2 size;
        // 纹理信息
        TextureInfo texture{};
        // 绘制层级，值越小越先绘制
        int zIndex{};
    };
    std::vector<Quad> mesh;
};

#endif  // MMM_TRANSFORMCOMPONENTS_HPP
