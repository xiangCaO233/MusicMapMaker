#ifndef MMM_QUADDATA_HPP
#define MMM_QUADDATA_HPP

#include <render/texture/TexMode.hpp>
#include <render/texture/TextureInfo.hpp>

struct QuadData {
    glm::vec2 pos;
    glm::vec2 size;
    glm::f32 rotation;
    glm::vec4 color;
    glm::vec2 uv_scale;
    glm::vec2 group_size;
    glm::uint32 layer_idx;
    glm::uint32 no_filter;
    TexAlignMode talign;
    TexScaleMode tscale;
};

#endif  // MMM_QUADDATA_HPP
