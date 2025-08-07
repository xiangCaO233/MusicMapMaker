#ifndef MMM_RENDERCOMMAND_HPP
#define MMM_RENDERCOMMAND_HPP

#include <render/QuadData.hpp>
#include <render/texture/TexMode.hpp>
#include <render/texture/TextureInfo.hpp>

struct BaseInfo {
    glm::vec2 pos;
    glm::vec2 size;
    glm::f32 rotation{0.f};
    glm::vec4 color{1.f};
    glm::uint32 no_filter{false};
};

struct RadiusInfo {
    // 圆角
    glm::vec2 radius{0.f};
    glm::f32 radius_effect_param{0.f};
    RadiusEffect radius_effect{RadiusEffect::FADE_IN_AND_OUT};
};

struct TexturesInfo {
    TextureInfo texture{};
    TexAlignMode talign{TexAlignMode::CENTER};
    TexScaleMode tscale{TexScaleMode::AUTO_SCALE_AND_CUT};
};

struct RenderCommand {
    BaseInfo baseInfo;

    RadiusInfo radiusInfo;

    TexturesInfo texturesInfo;

    QuadData to_data() const {
        return {
            baseInfo.pos + baseInfo.size / 2.f + radiusInfo.radius_effect_param,
            baseInfo.size + glm::vec2(2.f * radiusInfo.radius_effect_param),
            baseInfo.rotation,
            baseInfo.color,
            radiusInfo.radius,
            radiusInfo.radius_effect_param,
            radiusInfo.radius_effect,
            texturesInfo.texture.uv_scale,
            texturesInfo.texture.group_size,
            texturesInfo.texture.layer_index,
            baseInfo.no_filter,
            texturesInfo.talign,
            texturesInfo.tscale};
    }
};

struct RenderBatch {
    uint32_t texture_array_id;
    size_t startIndex;
    size_t instanceCount;
};

#endif  // MMM_RENDERCOMMAND_HPP
