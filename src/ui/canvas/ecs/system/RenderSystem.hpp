#ifndef MMM_NORMALRENDERSYSTEM_HPP
#define MMM_NORMALRENDERSYSTEM_HPP

#include <ecs/ECSCore.hpp>
#include <ecs/component/CoreComponents.hpp>
#include <ecs/component/NoteComponents.hpp>
#include <ecs/component/RelationComponents.hpp>
#include <ecs/component/TransformComponents.hpp>
#include <ecs/system/time2pixel/TimePixelConverter.hpp>
#include <info/MapCanvasInfo.hpp>
#include <layer/ILayer.hpp>
#include <mmm/map/MMap.hpp>
#include <util/mutil.hpp>
#include <vector>

class RenderSystem {
    struct RenderableQuad {
        GeneratedMesh::Quad quad;
        MeshState state;
    };

   public:
    void update(
        const entt::registry& registry,
        std::unordered_map<entt::entity, GeneratedMesh>& generated_meshes,
        const MapCanvasInfo* info, const TimePixelConverter& converter,
        ILayer* layer, RenderDataBuffer& buffer,
        bool is_preview = false) const {
        // 渲染普通物件
        // const auto& realtime_info = info->realTimeInfo;
        std::vector<RenderableQuad> all_mesh;
        // 遍历所有需要渲染的普通实体(除去虚影和即将删除的)
        if (is_preview) {
            auto view =
                registry
                    .view<TimeComponent, NoteComponent, TransformComponent>();
            // 收集网格
            for (auto& e : view) {
                auto& meshinfo = generated_meshes[e];
                auto& mesh = meshinfo.mesh;
                for (const auto& quad : mesh) {
                    // 网格整体状态绑定
                    all_mesh.push_back({quad, meshinfo.state});
                }
            }
        } else {
            auto view =
                registry.view<TimeComponent, NoteComponent, TransformComponent,
                              InMaintrackComponent>();
            // 收集网格
            for (auto& e : view) {
                auto& meshinfo = generated_meshes[e];
                auto& mesh = meshinfo.mesh;
                for (const auto& quad : mesh) {
                    // 网格整体状态绑定
                    all_mesh.push_back({quad, meshinfo.state});
                }
            }
        }

        // 排序网格
        std::sort(all_mesh.begin(), all_mesh.end(),
                  [](const RenderableQuad& quad1, const RenderableQuad& quad2) {
                      return quad1.quad.zIndex < quad2.quad.zIndex;
                  });

        auto& main_track_layout = info->editorInfo.track_layout;
        auto xpos = main_track_layout.x + main_track_layout.z;
        // 计算预览区轨道布局
        auto preview_tracks_rect =
            glm::vec4{xpos, 0, info->baseInfo.canvasSize.width() - xpos,
                      info->baseInfo.canvasSize.height()};

        // 生成网格的渲染指令
        for (int i{0}; i < all_mesh.size(); ++i) {
            auto& rquad = all_mesh[i];
            // 可见性检测
            if (!mutil::checkOverlap(
                    is_preview ? preview_tracks_rect : main_track_layout,
                    glm::vec4(rquad.quad.pos, rquad.quad.size)))
                continue;
            PrimitiveCommand cmd;
            cmd.cmdType = CommandType::PRIMITIVE;
            cmd.baseInfo.pos = rquad.quad.pos;
            cmd.baseInfo.size = rquad.quad.size;
            cmd.texturesInfo.texture = rquad.quad.texture;

            // 整个网格的附加状态
            switch (rquad.state) {
                case MeshState::GHOST: {
                    cmd.baseInfo.color = {1.f, 1.f, 1.f, .4f};
                    break;
                }
                case MeshState::GLOW: {
                    rquad.quad.state = PartState::GLOW;
                    break;
                }
                case MeshState::GLOW_AND_EMPHASIZE: {
                    cmd.baseInfo.color = {1.f, 1.f, .0f, .4f};
                    rquad.quad.state = PartState::GLOW;
                    break;
                }
                case MeshState::MARKDELETE: {
                    cmd.baseInfo.color = {1.f, .1f, .1f, .8f};
                }
                default:
                    break;
            }

            // 当前部分的发光状态
            switch (rquad.quad.state) {
                case PartState::GLOW: {
                    cmd.radiusInfo.radius_effect_param = 1.f;
                    cmd.radiusInfo.radius_effect = RadiusEffect::GLOWING;
                }
                default:
                    break;
            }

            buffer.add_PrimitiveCommand(cmd);
        }
    }
};

#endif  // MMM_NORMALRENDERSYSTEM_HPP
