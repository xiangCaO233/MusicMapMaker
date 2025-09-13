#ifndef MMM_MESHGENERATESYSTEM_HPP
#define MMM_MESHGENERATESYSTEM_HPP

#include <ecs/ECSCore.hpp>
#include <ecs/component/CoreComponents.hpp>
#include <ecs/component/NoteComponents.hpp>
#include <ecs/component/RelationComponents.hpp>
#include <ecs/component/TransformComponents.hpp>
#include <ecs/system/TimePixelConverter.hpp>
#include <info/MapCanvasInfo.hpp>
#include <layer/ILayer.hpp>
#include <map/skin/MSkin.hpp>

class MeshGenerateSystem {
   public:
    void update(const entt::registry& registry, MapCanvasInfo* info,
                const TimePixelConverter& converter,
                ToolInteractionState* tool_interaction_state,
                std::unordered_map<entt::entity, GeneratedMesh>&
                    out_generated_meshes) const {
        const auto& realtime_info = info->realTimeInfo;
        const glm::vec2 current_mouse_pos = {realtime_info.mousePos.x(),
                                             realtime_info.mousePos.y()};
        // 生成物件的网格组件
        // const auto& realtime_info = info->realTimeInfo;
        // 获取轨道布局信息
        const glm::vec4& all_tracks_rect = info->editorInfo.track_layout;
        const int track_count =
            info->editorInfo.map->base_metadata().track_count;
        if (track_count == 0) return;
        const auto single_track_width = all_tracks_rect.z / float(track_count);

        // 遍历所有需要生成网格的实体(除去组合物件的子键)
        auto view =
            registry.view<TimeComponent, NoteComponent, TransformComponent>(
                entt::exclude<ChildOfComponent>);
        for (auto& e : view) {
            assert(registry.valid(e) &&
                   "FATAL: Invalid entity handle detected!");
            // 填充目标网格属性
            auto& entity_mesh = out_generated_meshes[e];
            entity_mesh.source_entity = e;

            // 获取note原始详细信息
            const auto [time] = registry.get<TimeComponent>(e);
            const auto [track_index, handle] = registry.get<NoteComponent>(e);
            const auto [y] = registry.get<TransformComponent>(e);
            auto* ghost = registry.try_get<GhostComponent>(e);
            if (ghost) {
                auto drag_info = tool_interaction_state->getDragState();
                if (drag_info.drag_start_hit.part == NotePart::HEAD) {
                    // 若为头则计算并更新此时鼠标最近的分拍线时间作为物件时间
                    // 计算并更新此时鼠标最近的轨道
                }

                // 虚影方式渲染-根据工具交互状态确定如何渲染
                // 无需渲染悬浮效果
                generateMesh(std::nullopt, all_tracks_rect, track_index,
                             single_track_width, info, registry, e, entity_mesh,
                             time, y, converter);
            } else {
                // 获取悬浮信息
                auto hovered_info = tool_interaction_state->getHover();
                // 非虚影方式渲染-直接渲染原始物件
                generateMesh(hovered_info, all_tracks_rect, track_index,
                             single_track_width, info, registry, e, entity_mesh,
                             time, y, converter);
            }
        }
    }

   private:
    TextureInfo tex(const MapCanvasInfo* info, const entt::registry& registry,
                    const entt::entity& e, TexType type) const {
        TextureInfo texinfo;
        using enum TexType;
        using enum ObjectStatus;
        texinfo = info->editorInfo.skin->get_object_texture(type, COMMON);
        return texinfo;
    }

    void generateMesh(const std::optional<MeshPartInfo>& hovered_info,
                      const glm::vec4& all_tracks_rect, int32_t track_index,
                      const float& single_track_width, MapCanvasInfo*& info,
                      const entt::registry& registry, const entt::entity& e,
                      GeneratedMesh& entity_mesh, const uint32_t& time,
                      const float& y, const TimePixelConverter& converter,
                      bool ghost = false) const {
        // 判断悬浮情况
        auto hovered =
            hovered_info.has_value() && hovered_info.value().source_entity == e;
        auto hovered_part = hovered_info.has_value() ? hovered_info.value().part
                                                     : NotePart::NONE;

        // 根据note信息生成网格
        const float x = all_tracks_rect.x +
                        (float(track_index) + 0.5f) * single_track_width;

        // 获取头纹理
        TextureInfo head_texinfo =
            tex(info, registry, e,
                (registry.all_of<HoldComponent>(e) ||
                 registry.all_of<FlickComponent>(e) ||
                 registry.all_of<CompositeRootComponent>(e))
                    ? TexType::HOLD_HEAD
                    : TexType::NORMAL_NOTE);

        // 物件缩放
        const auto obj_scale = single_track_width / head_texinfo.origin_size.x;

        glm::vec2 head_size = obj_scale * head_texinfo.origin_size * 1.25f;

        // 共同的头网格(head在层级2(最上层))
        auto head_pos = glm::vec2(x - head_size.x / 2.f, y - head_size.y / 2.f);

        entity_mesh.mesh.emplace_back(
            head_pos, head_size, head_texinfo, NotePart::HEAD, ghost, 2,
            hovered && hovered_part == NotePart::HEAD);

        if (registry.all_of<HoldComponent>(e)) {
            entity_mesh.mesh.back().part = NotePart::HOLD_HEAD;
            entity_mesh.mesh.back().glow =
                hovered && hovered_part == NotePart::HOLD_HEAD;
            // 计算持续面身高度
            const auto& [duration] = registry.get<HoldComponent>(e);
            auto body_height =
                y - converter.timeToPixel(time + duration,
                                          info->realTimeInfo.current_time_info
                                              .presentation_canvas_time,
                                          info);

            // 获取面身纹理
            TextureInfo hold_body_texinfo =
                tex(info, registry, e, TexType::HOLD_BODY_VERTICAL);
            auto body_width = hold_body_texinfo.origin_size.x * obj_scale;

            // 绘制面身(画在面条结束的位置)
            // 面身网格(在层级0(最下层))
            auto body_pos = glm::vec2(x - body_width / 2.f, y - body_height);
            auto body_size = glm::vec2(body_width, body_height);

            entity_mesh.mesh.emplace_back(
                body_pos, body_size, hold_body_texinfo, NotePart::HOLD_BODY,
                ghost, 0, hovered && hovered_part == NotePart::HOLD_BODY,
                TexScaleMode::TILE_BASEWIDTH_REPEAT);

            // 绘制一个面尾网格(同样画在面条结束的位置)
            // 获取面尾纹理
            // 面尾网格(在层级1)
            TextureInfo hold_end_texinfo =
                tex(info, registry, e, TexType::HOLD_END);
            auto end_size = hold_end_texinfo.origin_size * obj_scale;
            auto end_pos = glm::vec2(x - end_size.x / 2.f,
                                     y - body_height - end_size.y / 2.f);
            entity_mesh.mesh.emplace_back(
                end_pos, end_size, hold_end_texinfo, NotePart::HOLD_END, ghost,
                1, hovered && hovered_part == NotePart::HOLD_END);

        } else if (registry.all_of<FlickComponent>(e)) {
            // 获取flick身纹理
            // TextureInfo flick_body_texinfo =
            //     tex(info, registry, e, TexType::HOLD_BODY_HORIZONTAL);
            // flick身网格(在层级0(最下层))
            // mesh.emplace_back(
            //     glm::vec2(x - (single_track_width / 2.f - 4.f), y - 15),
            //     glm::vec2(single_track_width - 8.f, 30),
            //     flick_body_texinfo, 0);
            // 绘制一个滑尾网格
        } else if (registry.all_of<CompositeRootComponent>(e)) {
            // 组合键-二级遍历
        }
    }
};
#endif  // MMM_MESHGENERATESYSTEM_HPP
