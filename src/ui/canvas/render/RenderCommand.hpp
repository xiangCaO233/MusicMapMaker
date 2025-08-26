#ifndef MMM_RENDERCOMMAND_HPP
#define MMM_RENDERCOMMAND_HPP

#include <render/GPUData.hpp>
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

enum class CommandType {
    QUAD,
    MESH,
};

struct CommandHandle {
    CommandType type;
    // 在各自指令表中的索引
    size_t index_in_pool;
};

struct RenderCommand {
    CommandType cmdType;
    TexturesInfo texturesInfo;
};

struct MeshCommand : public RenderCommand {
    // 网格数据
    MeshData to_data() const { return {}; };
};

struct QuadCommand : public RenderCommand {
    BaseInfo baseInfo;
    RadiusInfo radiusInfo;

    QuadData to_data() const {
        return QuadData{
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
    CommandType type;
    uint32_t texture_array_id;
    // 当渲染类型为矩形时,这是矩形实例的起始索引
    // 当渲染类型为网格时,这是顶点的起始索引
    size_t startIndex;
    // 当渲染类型为矩形时,这是矩形实例的个数
    // 当渲染类型为网格时,这是顶点的个数
    size_t elementCount;
};

#endif  // MMM_RENDERCOMMAND_HPP
