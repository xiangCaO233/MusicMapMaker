#ifndef MMM_GPUDATA_HPP
#define MMM_GPUDATA_HPP

#include <render/texture/TexMode.hpp>
#include <render/texture/TextureInfo.hpp>
#include <vector>

struct CustomVertex {
    glm::vec2 pos;
    glm::vec4 color;
    glm::vec2 uv;
};

struct MeshData {
    std::vector<CustomVertex> vertices;

    // 纹理
    glm::vec2 uv_scale;
    glm::vec2 group_size;
    glm::int32 layer_idx;

    // 蒙版效果
    glm::uint32 no_filter;

    // 纹理选项
    TexAlignMode talign;
    TexScaleMode tscale;
};

struct QuadData {
    // 基本信息
    glm::vec2 pos;
    glm::vec2 size;
    glm::f32 rotation;
    glm::vec4 color;

    // 圆角
    glm::vec2 radius;
    glm::f32 radius_effect_param;
    RadiusEffect radius_effect;

    // 纹理
    glm::vec2 uv_scale;
    glm::vec2 group_size;
    glm::int32 layer_idx;

    // 蒙版效果
    glm::uint32 no_filter;

    // 纹理选项
    TexAlignMode talign;
    TexScaleMode tscale;
};

#endif  // MMM_GPUDATA_HPP
