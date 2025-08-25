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
    void update(ECSCore& core, const MapCanvasInfo* info,
                TimePixelConverter& converter) const {
        // 生成物件的网格组件
        auto& registry = core.ecs_registry();
        // const auto& realtime_info = info->realTimeInfo;
        // 获取轨道布局信息
        const glm::vec4& all_tracks_rect = info->editorInfo.track_layout;
        const int track_count =
            info->editorInfo.map->base_metadata().track_count;
        if (track_count == 0) return;
        const float single_track_width = all_tracks_rect.z / float(track_count);
        // 遍历所有需要生成网格的实体(除去组合物件的子键)
        auto view =
            registry.view<TimeComponent, NoteComponent, TransformComponent_1>(
                entt::exclude<ChildOfComponent>);
        for (auto& e : view) {
            const auto& [time] = registry.get<TimeComponent>(e);
            // 清理上一帧的网格数据
            auto& mesh = registry.get_or_emplace<TransformComponent_2>(e).mesh;
            mesh.clear();
            // --- 根据Note类型进行分支处理 ---
            const auto& [track_index, handle] = registry.get<NoteComponent>(e);
            const auto& [y] = registry.get<TransformComponent_1>(e);
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
                (single_track_width - 8.f) / head_texinfo.origin_size.x;

            glm::vec2 head_size = {single_track_width - 8.f,
                                   obj_scale * head_texinfo.origin_size.y};

            // 共同的头网格(head在层级1)
            mesh.emplace_back(
                glm::vec2(x - head_size.x / 2.f, y - head_size.y / 2.f),
                head_size, head_texinfo, 1);

            if (registry.all_of<HoldComponent>(e)) {
                // 计算持续面身高度
                const auto& [duration] = registry.get<HoldComponent>(e);
                auto body_height = converter.timeToPixel(
                    duration, info->realTimeInfo.presentation_canvas_time);

                // 获取面身纹理
                TextureInfo hold_body_texinfo =
                    tex(info, registry, e, TexType::HOLD_BODY_VERTICAL);
                auto body_width = hold_body_texinfo.origin_size.x * obj_scale;

                // 绘制面身(画在面条结束的位置)
                // 面身网格(在层级0(最下层))
                mesh.emplace_back(
                    glm::vec2(x - body_width / 2.f, y - body_height),
                    glm::vec2{body_width, body_height}, hold_body_texinfo, 0);

                // 绘制一个面尾网格(同样画在面条结束的位置)
                // 获取面尾纹理
                // 面尾网格(在层级1)
                TextureInfo hold_end_texinfo =
                    tex(info, registry, e, TexType::HOLD_END);
                auto end_size = hold_end_texinfo.origin_size * obj_scale;
                mesh.emplace_back(glm::vec2(x - end_size.x / 2.f,
                                            y - body_height - end_size.y / 2.f),
                                  end_size, hold_end_texinfo, 1);
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
        // 自动区分状态
        if (registry.all_of<HoveredComponent>(e)) {
            texinfo = info->editorInfo.skin->get_object_texture(type, HOVER);
        } else if (registry.all_of<SelectedComponent>(e)) {
            texinfo = info->editorInfo.skin->get_object_texture(type, SELECTED);
        } else {
            texinfo = info->editorInfo.skin->get_object_texture(type, COMMON);
        }
        return texinfo;
    }
};
#endif  // MMM_MESHGENERATESYSTEM_HPP
