#ifndef MMM_TRANSFORMCOMPONENTS_HPP
#define MMM_TRANSFORMCOMPONENTS_HPP

#include <entt.hpp>
#include <glm/glm.hpp>
#include <info/NotePart.hpp>
#include <render/texture/TexMode.hpp>
#include <render/texture/TextureInfo.hpp>
#include <sstream>
#include <vector>

// 转换组件1-指示此物件在画布中的逻辑中心位置
struct TransformComponent {
    float main_y;
    float preview_y;
};

// 整个网格的状态
enum class MeshState {
    NONE,
    GHOST,
    GLOW,
    GLOW_AND_EMPHASIZE,
    MARKDELETE,
};

// 网格内部分的状态
enum class PartState {
    NONE,
    GLOW,
};

struct GeneratedMesh {
    // 标记这个网格属于哪个实体(若为子物件则此为父实体)
    entt::entity source_entity;
    // (若为子物件则此为子实体)
    entt::entity child_entity{entt::null};
    struct Quad {
        glm::vec2 pos;
        glm::vec2 size;
        // 纹理信息
        TextureInfo texture{};

        // 本矩形对应的物件部位
        NotePart part;

        // 部分状态
        PartState state;

        // 绘制层级，值越小越先绘制
        int zIndex{0};

        // 纹理缩放模式
        TexScaleMode mode{TexScaleMode::FORCE_FILL};
    };
    std::vector<Quad> mesh;
    // 网格状态
    MeshState state;
};

// --- to_string 函数实现 ---
inline std::string to_string(const GeneratedMesh& generatedMesh) {
    std::stringstream ss;
    ss << "GeneratedMesh:\n";
    // 打印源实体ID
    // 使用 static_cast<uint32_t> 打印 entt::entity 的底层整数值
    ss << "  source_entity: "
       << static_cast<uint32_t>(generatedMesh.source_entity) << "\n";
    ss << "  quad_count: " << generatedMesh.mesh.size() << "\n";

    // 遍历并打印每个 Quad 的信息
    for (size_t i = 0; i < generatedMesh.mesh.size(); ++i) {
        const auto& quad = generatedMesh.mesh[i];
        ss << "    --- Quad #" << i << " ---\n";
        ss << "    pos: {" << quad.pos.x << ", " << quad.pos.y << "}\n";
        ss << "    size: {" << quad.size.x << ", " << quad.size.y << "}\n";
        ss << "    zIndex: " << quad.zIndex << "\n";
        ss << "    glow: " << (quad.state == PartState::GLOW ? "true" : "false")
           << "\n";
        ss << "    part: " << to_string(quad.part) << "\n";
    }

    return ss.str();
}

#endif  // MMM_TRANSFORMCOMPONENTS_HPP
