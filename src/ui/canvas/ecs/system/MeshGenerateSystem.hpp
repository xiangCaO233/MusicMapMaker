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

            // 获取悬浮信息
            auto hovered_info = tool_interaction_state->getHover();
            auto* ghost = registry.try_get<GhostComponent>(e);
            auto* delmark = registry.try_get<DeleteMarkComponent>(e);
            if (delmark) {
                // 标记删除方式渲染
                entity_mesh.state = MeshState::MARKDELETE;
                generateMesh(hovered_info, all_tracks_rect, track_index,
                             single_track_width, info, registry, e, entity_mesh,
                             time, y, converter);
            } else if (ghost) {
                // 虚影方式渲染-根据工具交互状态确定如何渲染
                entity_mesh.state = MeshState::GHOST;
                auto drag_info = tool_interaction_state->getDragState();
                auto mousePressPos =
                    tool_interaction_state->getMouseState().press_pos;
                auto mousePos =
                    tool_interaction_state->getMouseState().current_pos;
                if (drag_info.drag_start_hit.part == NotePart::HEAD) {
                    // 若为头则计算并更新此时鼠标最近的分拍线时间作为物件时间
                    // 计算并更新此时鼠标最近的轨道
                }

                generateMesh(hovered_info, all_tracks_rect, track_index,
                             single_track_width, info, registry, e, entity_mesh,
                             time, y, converter);
            } else {
                // 非虚影方式渲染-直接渲染原始物件
                generateMesh(hovered_info, all_tracks_rect, track_index,
                             single_track_width, info, registry, e, entity_mesh,
                             time, y, converter);
            }
        }
    }

   private:
    TextureInfo tex(const MapCanvasInfo* info, TexType type) const {
        TextureInfo texinfo;
        using enum TexType;
        using enum ObjectStatus;
        texinfo = info->editorInfo.skin->get_object_texture(type, COMMON);
        return texinfo;
    }

    void generateHeadMesh(const std::optional<MeshPartInfo>& hovered_info,
                          const entt::entity& e, GeneratedMesh& entity_mesh,
                          const float& x, const float& y,
                          const glm::vec2& head_pos, const glm::vec2& head_size,
                          const TextureInfo& head_texinfo) const {
        // 判断悬浮情况
        auto hovered_entity =
            hovered_info.has_value() && hovered_info.value().source_entity == e;

        // 是否悬浮在头部-普通物件上或复合物件的头部
        auto hovered_head =
            hovered_entity &&
            (hovered_info.value().part == NotePart::SLIDE_HEAD ||
             hovered_info.value().part == NotePart::HOLD_HEAD ||
             hovered_info.value().part == NotePart::HEAD);

        entity_mesh.mesh.emplace_back(
            head_pos, head_size, head_texinfo, NotePart::HEAD,
            hovered_head ? PartState::GLOW : PartState::NONE, 2);
    }

    void generateHoldBodyAndTailMesh(
        const std::optional<MeshPartInfo>& hovered_info,
        const entt::registry& registry, const entt::entity& e,
        GeneratedMesh& entity_mesh, const uint32_t& time,
        const float& obj_scale, MapCanvasInfo*& info,
        const TimePixelConverter& converter, const float& x,
        const float& y) const {
        // 更新物件头部位为更精确的位置
        entity_mesh.mesh.back().part = NotePart::HOLD_HEAD;
        // 判断悬浮情况
        auto hovered_entity =
            hovered_info.has_value() && hovered_info.value().source_entity == e;

        // 是否悬浮在面身部分
        auto hovered_hold_body =
            hovered_entity && hovered_info.value().part == NotePart::HOLD_BODY;

        // 计算持续面身高度
        const auto& [duration] = registry.get<HoldComponent>(e);
        auto body_height =
            y -
            converter.timeToPixel(
                time + duration,
                info->realTimeInfo.current_time_info.presentation_canvas_time,
                info);

        // 获取面身纹理
        TextureInfo hold_body_texinfo = tex(info, TexType::HOLD_BODY_VERTICAL);

        auto body_width = hold_body_texinfo.origin_size.x * obj_scale;

        // 绘制面身(画在面条结束的位置)
        // 面身网格(在层级0(最下层))
        auto body_pos = glm::vec2(x - body_width / 2.f, y - body_height);
        auto body_size = glm::vec2(body_width, body_height);

        entity_mesh.mesh.emplace_back(
            body_pos, body_size, hold_body_texinfo, NotePart::HOLD_BODY,
            hovered_hold_body ? PartState::GLOW : PartState::NONE, 0,
            // 面身为重复绘制纹理模式(基于给定宽)
            TexScaleMode::TILE_BASEWIDTH_REPEAT);

        // 绘制一个面尾网格(同样画在面条结束的位置)
        // 获取面尾纹理
        // 面尾网格(在层级1)
        // 是否悬浮在面尾部分
        auto hovered_hold_tail =
            hovered_entity && hovered_info.value().part == NotePart::HOLD_END;
        TextureInfo hold_end_texinfo = tex(info, TexType::HOLD_END);
        auto end_size = hold_end_texinfo.origin_size * obj_scale;
        auto end_pos =
            glm::vec2(x - end_size.x / 2.f, y - body_height - end_size.y / 2.f);
        entity_mesh.mesh.emplace_back(
            end_pos, end_size, hold_end_texinfo, NotePart::HOLD_END,
            hovered_hold_tail ? PartState::GLOW : PartState::NONE, 1);
    }

    void generateMesh(const std::optional<MeshPartInfo>& hovered_info,
                      const glm::vec4& all_tracks_rect, int32_t track_index,
                      const float& single_track_width, MapCanvasInfo*& info,
                      const entt::registry& registry, const entt::entity& e,
                      GeneratedMesh& entity_mesh, const uint32_t& time,
                      const float& y,
                      const TimePixelConverter& converter) const {
        // 根据note信息生成网格

        // 物件头的x轴位置(中心)
        const float x = all_tracks_rect.x +
                        (float(track_index) + 0.5f) * single_track_width;

        // 头的纹理选择
        TextureInfo head_texinfo =
            tex(info, (registry.all_of<HoldComponent>(e) ||
                       registry.all_of<FlickComponent>(e) ||
                       registry.all_of<CompositeRootComponent>(e))
                          ? TexType::HOLD_HEAD
                          : TexType::NORMAL_NOTE);

        // 物件整体缩放计算(统一放大1.25x)
        const auto obj_scale =
            single_track_width / head_texinfo.origin_size.x * 1.25f;

        glm::vec2 head_size = obj_scale * head_texinfo.origin_size;

        // 共同的头网格(head在层级2(最上层))
        auto head_pos = glm::vec2(x - head_size.x / 2.f, y - head_size.y / 2.f);
        generateHeadMesh(hovered_info, e, entity_mesh, x, y, head_pos,
                         head_size, head_texinfo);

        if (registry.all_of<HoldComponent>(e)) {
            generateHoldBodyAndTailMesh(hovered_info, registry, e, entity_mesh,
                                        time, obj_scale, info, converter, x, y);
        } else if (registry.all_of<FlickComponent>(e)) {
            // 获取flick身纹理
            // 绘制一个滑尾网格
        } else if (registry.all_of<CompositeRootComponent>(e)) {
            // 组合键-二级遍历
        }
    }
};
#endif  // MMM_MESHGENERATESYSTEM_HPP
