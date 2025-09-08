#ifndef MMM_MESHGENERATESYSTEM_HPP
#define MMM_MESHGENERATESYSTEM_HPP

#include <ecs/ECSCore.hpp>
#include <ecs/component/CoreComponents.hpp>
#include <ecs/component/NoteComponents.hpp>
#include <ecs/component/RelationComponents.hpp>
#include <ecs/component/StateComponents.hpp>
#include <ecs/component/TransformComponents.hpp>
#include <ecs/system/TimePixelConverter.hpp>
#include <info/MapCanvasInfo.hpp>
#include <layer/ILayer.hpp>
#include <map/skin/MSkin.hpp>

class MeshGenerateSystem {
   public:
    void update(const entt::registry& registry, MapCanvasInfo* info,
                const TimePixelConverter& converter,
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
        const float single_track_width = all_tracks_rect.z / float(track_count);
        // 遍历所有需要生成网格的实体(除去组合物件的子键)
        auto view =
            registry.view<TimeComponent, NoteComponent, TransformComponent>(
                entt::exclude<ChildOfComponent>);
        for (auto& e : view) {
            auto& entity_mesh = out_generated_meshes[e];
            entity_mesh.source_entity = e;
            bool hovered{false};

            const auto& [time] = registry.get<TimeComponent>(e);
            // 清理上一帧的网格数据
            // auto& mesh =
            // registry.get_or_emplace<TransformComponent_2>(e).mesh;
            // mesh.clear();
            // --- 根据Note类型进行分支处理 ---
            const auto& [track_index, handle] = registry.get<NoteComponent>(e);
            const auto& [y] = registry.get<TransformComponent>(e);
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
            const auto obj_scale =
                single_track_width / head_texinfo.origin_size.x;

            glm::vec2 head_size = obj_scale * head_texinfo.origin_size * 1.25f;

            // 共同的头网格(head在层级2(最上层))
            auto head_pos =
                glm::vec2(x - head_size.x / 2.f, y - head_size.y / 2.f);
            bool hoverd_head =
                is_hover_part(head_pos, head_size, current_mouse_pos);
            if (hoverd_head) {
                info->realTimeInfo.hovered_info.part = HoverPart::HEAD;
            }
            entity_mesh.mesh.emplace_back(head_pos, head_size, head_texinfo, 2,
                                          hoverd_head);
            // 若有悬停到头部则确定悬停属性
            hovered = hovered || hoverd_head;

            if (registry.all_of<HoldComponent>(e)) {
                // 计算持续面身高度
                const auto& [duration] = registry.get<HoldComponent>(e);
                auto body_height =
                    y - converter.timeToPixel(
                            time + duration,
                            info->realTimeInfo.presentation_canvas_time, info);

                // 获取面身纹理
                TextureInfo hold_body_texinfo =
                    tex(info, registry, e, TexType::HOLD_BODY_VERTICAL);
                auto body_width = hold_body_texinfo.origin_size.x * obj_scale;

                // 绘制面身(画在面条结束的位置)
                // 面身网格(在层级0(最下层))
                auto body_pos =
                    glm::vec2(x - body_width / 2.f, y - body_height);
                auto body_size = glm::vec2(body_width, body_height);
                bool hoverd_body =
                    is_hover_part(body_pos, body_size, current_mouse_pos);
                if (hoverd_body) {
                    info->realTimeInfo.hovered_info.part = HoverPart::HOLD_BODY;
                }
                entity_mesh.mesh.emplace_back(
                    body_pos, body_size, hold_body_texinfo, 0, hoverd_body,
                    TexScaleMode::TILE_BASEWIDTH_REPEAT);

                // 绘制一个面尾网格(同样画在面条结束的位置)
                // 获取面尾纹理
                // 面尾网格(在层级1)
                TextureInfo hold_end_texinfo =
                    tex(info, registry, e, TexType::HOLD_END);
                auto end_size = hold_end_texinfo.origin_size * obj_scale;
                auto end_pos = glm::vec2(x - end_size.x / 2.f,
                                         y - body_height - end_size.y / 2.f);
                bool hoverd_end =
                    is_hover_part(end_pos, end_size, current_mouse_pos);
                if (hoverd_end) {
                    info->realTimeInfo.hovered_info.part = HoverPart::HOLD_END;
                }
                entity_mesh.mesh.emplace_back(end_pos, end_size,
                                              hold_end_texinfo, 1, hoverd_end);
                // 若有悬停到中间部则确定悬停属性
                // 若有悬停到尾部则确定悬停属性
                hovered = hovered || hoverd_body || hoverd_end;
                info->realTimeInfo.hovered_info.has_hovered_entity = hovered;
                if (hovered) {
                    info->realTimeInfo.hovered_info.e = e;
                }

            } else if (registry.all_of<FlickComponent>(e)) {
                // 获取flick身纹理
                // TextureInfo flick_body_texinfo =
                //     tex(info, registry, e, TexType::HOLD_BODY_HORIZONTAL);
                // // flick身网格(在层级0(最下层))
                // mesh.emplace_back(
                //     glm::vec2(x - (single_track_width / 2.f - 4.f), y - 15),
                //     glm::vec2(single_track_width - 8.f, 30),
                //     flick_body_texinfo, 0);
                // 绘制一个滑尾网格
            } else if (registry.all_of<CompositeRootComponent>(e)) {
                // 组合键-二级遍历
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

    bool is_hover_part(const glm::vec2& pos, const glm::vec2& size,
                       const glm::vec2& mouse_pos) const {
        // 检查鼠标的 x 坐标是否在矩形的水平范围内。
        bool x_in_range = mouse_pos.x >= pos.x && mouse_pos.x < pos.x + size.x;

        // 检查鼠标的 y 坐标是否在矩形的垂直范围内。
        // 注意：在许多图形系统中，y 轴是倒置的（0 在顶部）。
        // 此实现假设一个标准的坐标系。如果你的 y 轴是倒置的，可能需要调整。
        bool y_in_range = mouse_pos.y >= pos.y && mouse_pos.y < pos.y + size.y;

        // 只有当 x 和 y 坐标都在范围内时，鼠标才悬停在该部分上。
        return x_in_range && y_in_range;
    }
};
#endif  // MMM_MESHGENERATESYSTEM_HPP
