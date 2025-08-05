#ifndef MMM_RENDERCOMMAND_HPP
#define MMM_RENDERCOMMAND_HPP

#include <glm/fwd.hpp>
#include <render/quad/QuadData.hpp>
#include <render/texture/TexMode.hpp>
#include <render/texture/TextureInfo.hpp>

struct RenderCommand {
    glm::vec2 pos;
    glm::vec2 size;
    glm::f32 rotation;
    glm::vec4 color;
    TextureInfo texture;
    TexAlignMode talign;
    TexScaleMode tscale;

    QuadData to_data() const {
        return {pos,
                size,
                rotation,
                color,
                texture.uv_scale,
                texture.layer_index,
                talign,
                tscale};
    }
};

struct RenderBatch {
    uint32_t texture_array_id;
    size_t startIndex;
    size_t instanceCount;
};

#endif  // MMM_RENDERCOMMAND_HPP
