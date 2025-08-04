#ifndef MMM_QUADINSTANCE_HPP
#define MMM_QUADINSTANCE_HPP

#include <glm/glm.hpp>
#include <memory>
#include <render/texture/TextureInstance.hpp>

#include "render/texture/TexMode.hpp"

struct QuadInstance {
    // 是否为字符矩形
    bool m_is_character_quad{false};

    // 矩形位置
    glm::vec2 m_position{0.f, 0.f};

    // 矩形尺寸-x倍率(width),y倍率(height)
    glm::vec2 m_scale{1.f, 1.f};

    // 旋转角度
    float m_rotation{0.f};

    // 使用的纹理
    std::shared_ptr<TextureInstance> m_texture{nullptr};

    // 纹理贴图方式
    TexScaleMode m_scalemode;
    TexAlignMode m_alignmode;
};
#endif  // MMM_QUADINSTANCE_HPP
