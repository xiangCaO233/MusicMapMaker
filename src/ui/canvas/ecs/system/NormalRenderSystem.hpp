#ifndef MMM_NORMALRENDERSYSTEM_HPP
#define MMM_NORMALRENDERSYSTEM_HPP

#include <algorithm>
#include <ecs/ECSCore.hpp>
#include <ecs/component/CoreComponents.hpp>
#include <ecs/component/NoteComponents.hpp>
#include <ecs/component/TransformComponents.hpp>
#include <ecs/system/TimePixelConverter.hpp>
#include <info/MapCanvasInfo.hpp>
#include <layer/ILayer.hpp>

class NormalRenderSystem {
   public:
    void update(ECSCore& core, const MapCanvasInfo* info,
                const TimePixelConverter& converter,
                ILayer::RenderDataBuffer& buffer) const {
        // 渲染普通物件
        auto& registry = core.ecs_registry();
        // const auto& realtime_info = info->realTimeInfo;

        // 遍历所有需要渲染的普通实体(除去虚影和即将删除的)
        auto view = registry.view<TransformComponent_2>(
            entt::exclude<GhostComponent, DeleteMarkComponent>);
        for (auto& e : view) {
            auto& [mesh] = view.get<TransformComponent_2>(e);
            // 排序网格
            std::sort(mesh.begin(), mesh.end(),
                      [](const TransformComponent_2::Quad& quad1,
                         const TransformComponent_2::Quad& quad2) {
                          return quad1.zIndex < quad2.zIndex;
                      });
            // 生成指令
            RenderCommand cmd;
            for (auto& quad : mesh) {
                cmd.baseInfo.pos = quad.pos;
                cmd.baseInfo.size = quad.size;
                cmd.texturesInfo.texture = quad.texture;
            }
            buffer.push_back(cmd);
        }
    }
};

#endif  // MMM_NORMALRENDERSYSTEM_HPP
