#ifndef MMM_GPUDATA_HPP
#define MMM_GPUDATA_HPP

#include <render/texture/TexMode.hpp>
#include <render/texture/TextureInfo.hpp>
#include <vector>

/*
 * 顶点位置
 * layout(location = 0) in vec2 vPosition;
 * 顶点uv
 * layout(location = 1) in vec2 vTexCoord;
 * 顶点颜色
 * layout(location = 2) in vec4 vColor;
 *
 * 顶点纹理信息
 * 顶点使用的纹理在层中的尺寸比例
 * layout(location = 3) in vec2 aUVScale;
 * 顶点使用的纹理的textureArray的统一尺寸
 * layout(location = 4) in vec2 aGroupSize;
 * 顶点使用的纹理在层中层索引
 * layout(location = 5) in int aTextureLayerIdx;
 * 顶点使用的纹理是否应用ubo蒙版效果
 * layout(location = 6) in uint aNoFilter;
 */

struct CustomVertex {
    // 基本顶点信息
    glm::vec2 pos;
    glm::vec2 uv;
    glm::vec4 color;
    // 纹理
    glm::vec2 uv_scale;
    glm::vec2 group_size;
    glm::int32 layer_idx;
    // 蒙版效果
    glm::uint32 no_filter;
};

struct PointsData {
    std::vector<CustomVertex> vertices;
};

/*
 * layout(location = 0) in vec2 aPosition;
 * layout(location = 1) in vec2 aScale;
 * layout(location = 2) in float aRotation;
 * layout(location = 3) in vec4 aColor;
 *
 * 圆角* layout(location = 4) in vec2 aRadius;
 * layout(location = 5) in float aRadiusEffectParam;
 * layout(location = 6) in uint aRadiusEffect;
 * 纹理未必占满采样器数组的一个layer
 * layout(location = 7) in vec2 aUVScale;
 * 但是一定是在0,0开始填充的,所以UVoffset恒为0,0
 * 贴图所在纹理组的组尺寸
 * layout(location = 8) in vec2 aGroupSize;
 * 纹理ID
 * layout(location = 9) in int aTextureLayerIdx;
 * 是否禁止蒙版效果
 * layout(location = 10) in uint aNoFilter;
 * 贴图策略
 * layout(location = 11) in uint aTexAlignStratergy;
 * layout(location = 12) in uint aTexScaleStratergy;
 */

struct PrimitiveData {
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
