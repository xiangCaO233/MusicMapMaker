#ifndef MMM_RENDERCOMMAND_HPP
#define MMM_RENDERCOMMAND_HPP

#include <render/command/GPUData.hpp>
#include <render/texture/TexMode.hpp>
#include <render/texture/TextureInfo.hpp>
#include <vector>

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
    // 矩形
    QUAD,
    // 图元
    PRIMITIVE,
    // 曲线
    CURVE,
    // 网格
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

struct MeshVertex {
    glm::vec2 vPos;
    glm::vec2 vUV;
    glm::vec4 vColor;
    // 在点集合中的位置(0内部,1头部,-1尾部)
    glm::int32 group_pos;
};

struct PointsCommnad : public RenderCommand {
    // 顶点列表
    std::vector<MeshVertex> vertices;
    // 是否应用蒙版效果
    glm::uint32 no_filter{false};
};

struct MeshCommand : public PointsCommnad {
    // 顶点索引列表
    std::vector<size_t> indicies;
    // 转换为网格数据
    // PointsData to_data() const {
    //     PointsData data;
    //     for (const auto& vIndex : indicies) {
    //         const auto& mesh_vertex = vertices[vIndex];
    //         // CustomVertex vertex{mesh_vertex.vPos,
    //         //                     mesh_vertex.vUV,
    //         //                     mesh_vertex.vColor,
    //         //                     texturesInfo.texture.uv_scale,
    //         //                     texturesInfo.texture.group_size,
    //         //                     texturesInfo.texture.layer_index,
    //         //                     no_filter};
    //         data.vertices.emplace_back(
    //             mesh_vertex.vPos, mesh_vertex.vUV, mesh_vertex.vColor,
    //             texturesInfo.texture.uv_scale,
    //             texturesInfo.texture.group_size,
    //             texturesInfo.texture.layer_index, no_filter,
    //             mesh_vertex.group_pos);
    //     }
    //     return data;
    // };
};

struct QuadCommand : public RenderCommand {
    BaseInfo baseInfo;
    RadiusInfo radiusInfo;

    // 转换为矩形数据
    // PrimitiveData to_data() const {
    //     return PrimitiveData{
    //         baseInfo.pos + baseInfo.size / 2.f +
    //         radiusInfo.radius_effect_param, baseInfo.size + glm::vec2(2.f *
    //         radiusInfo.radius_effect_param), baseInfo.rotation,
    //         baseInfo.color,
    //         radiusInfo.radius,
    //         radiusInfo.radius_effect_param,
    //         radiusInfo.radius_effect,
    //         texturesInfo.texture.uv_scale,
    //         texturesInfo.texture.group_size,
    //         texturesInfo.texture.layer_index,
    //         baseInfo.no_filter,
    //         texturesInfo.talign,
    //         texturesInfo.tscale,
    //         PrimitiveType::QUAD};
    // }
};

struct PrimitiveCommand : public RenderCommand {
    BaseInfo baseInfo;
    RadiusInfo radiusInfo;
    PrimitiveType primitive;

    // 转换为图元数据
    // PrimitiveData to_data() const {
    //     return PrimitiveData{
    //         baseInfo.pos + baseInfo.size / 2.f +
    //         radiusInfo.radius_effect_param, baseInfo.size + glm::vec2(2.f *
    //         radiusInfo.radius_effect_param), baseInfo.rotation,
    //         baseInfo.color,
    //         radiusInfo.radius,
    //         radiusInfo.radius_effect_param,
    //         radiusInfo.radius_effect,
    //         texturesInfo.texture.uv_scale,
    //         texturesInfo.texture.group_size,
    //         texturesInfo.texture.layer_index,
    //         baseInfo.no_filter,
    //         texturesInfo.talign,
    //         texturesInfo.tscale,
    //         primitive};
    // }
};

struct CurveCommand : public PointsCommnad {
    // 转换为曲线采样点数据
    // PointsData to_data() const {
    //     PointsData data;
    //     for (const auto& vertex : vertices) {
    //         data.vertices.emplace_back(
    //             vertex.vPos, vertex.vUV, vertex.vColor,
    //             texturesInfo.texture.uv_scale,
    //             texturesInfo.texture.group_size,
    //             texturesInfo.texture.layer_index, no_filter,
    //             vertex.group_pos);
    //     }
    //     return data;
    // };
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
