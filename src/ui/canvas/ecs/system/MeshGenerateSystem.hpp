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
    struct MapAxis {
        int64_t time{0};
        int64_t mousetime{0};
        int64_t track{0};
        int64_t x{0};
        int64_t y{0};
    };
    glm::vec2 current_mouse_pos;
    glm::vec4 all_tracks_rect;
    int track_count;
    uint32_t maplength;
    float judgeline_absolute_y;
    float single_track_width;
    float canvas_height;
    double presentation_canvas_time;
    std::optional<MeshPartInfo> hovered_info;
    MapCanvasInfo* info;
    RealTimeInfo* realtime_info;
    ToolInteractionState* tool_interaction_state;
    const TimePixelConverter* converter;
    const entt::registry* registry;

   public:
    MeshGenerateSystem() = default;
    void update(
        const entt::registry& registry_ref, MapCanvasInfo* mapinfo,
        const TimePixelConverter& converter_ref,
        ToolInteractionState* toolInteractionState,
        std::unordered_map<entt::entity, GeneratedMesh>& out_generated_meshes) {
        // 更新缓存信息
        registry = &registry_ref;
        info = mapinfo;
        realtime_info = &info->realTimeInfo;
        current_mouse_pos =
            glm::vec2{realtime_info->mousePos.x(), realtime_info->mousePos.y()};
        canvas_height = info->baseInfo.canvasSize.height();
        judgeline_absolute_y = canvas_height * info->baseInfo.judgeline_pos;
        tool_interaction_state = toolInteractionState;
        converter = &converter_ref;
        presentation_canvas_time =
            info->realTimeInfo.current_time_info.presentation_canvas_time;

        // 生成物件的网格组件
        // const auto& realtime_info = info->realTimeInfo;
        // 获取轨道布局信息
        all_tracks_rect = info->editorInfo.track_layout;
        track_count = info->editorInfo.map->base_metadata().track_count;
        maplength = info->editorInfo.map->base_metadata().map_length;
        if (track_count == 0) return;
        single_track_width = all_tracks_rect.z / float(track_count);
        // 获取悬浮信息
        hovered_info = tool_interaction_state->getHover();

        // 遍历所有需要生成网格的实体(除去组合物件的子键)
        auto view =
            registry->view<TimeComponent, NoteComponent, TransformComponent>(
                entt::exclude<ChildOfComponent>);
        for (auto& e : view) {
            // assert(registry->valid(e) &&
            //        "FATAL: Invalid entity handle detected!");
            // 填充目标网格属性
            auto& entity_mesh = out_generated_meshes[e];
            entity_mesh.source_entity = e;

            // 获取note原始详细信息
            auto [time] = registry->get<TimeComponent>(e);
            auto [track_index, handle] = registry->get<NoteComponent>(e);
            auto [y] = registry->get<TransformComponent>(e);

            auto* ghost = registry->try_get<GhostComponent>(e);
            auto* delmark = registry->try_get<DeleteMarkComponent>(e);
            if (delmark) {
                // 标记删除方式渲染
                entity_mesh.state = MeshState::MARKDELETE;
                generateMesh(track_index, e, entity_mesh, time, y);
            } else if (ghost) {
                // 虚影方式渲染-根据工具交互状态确定如何渲染

                // --------------------物件头拖动交互--------------------------
                entity_mesh.state = MeshState::GHOST;
                auto drag_info = tool_interaction_state->getDragState();
                auto mousePressPos =
                    tool_interaction_state->getMouseState().press_pos;
                auto mousePos =
                    tool_interaction_state->getMouseState().current_pos;
                // auto mouse_time =
                //     converter.pixelToTime(info->baseInfo.canvasSize.height()
                //     -
                //                               mousePos.y -
                //                               judgeline_absolute_y,
                //                           info->realTimeInfo.current_time_info
                //                               .presentation_canvas_time);
                if (drag_info.drag_start_hit.part == NotePart::HEAD ||
                    drag_info.drag_start_hit.part == NotePart::HOLD_HEAD) {
                    // 若为头则计算此时鼠标最近的分拍线时间作为物件时间
                    // 计算此时鼠标最近的轨道
                    auto axis = getPixelMapAxis(mousePos);

                    // 验证更新
                    auto validity = axis.time >= 0 && axis.time <= maplength &&
                                    axis.track >= 0 && axis.track < track_count;
                    toolInteractionState->setDragValidity(validity);
                    drag_info = tool_interaction_state->getDragState();
                    if (drag_info.is_valid) {
                        time = axis.time;
                        track_index = axis.track;
                        y = axis.y;
                    }
                }

                generateMesh(track_index, e, entity_mesh, time, y);
            } else {
                // 非虚影方式渲染-直接渲染原始物件
                generateMesh(track_index, e, entity_mesh, time, y);
            }
        }
    }

   private:
    TextureInfo tex(TexType type) const {
        TextureInfo texinfo;
        using enum TexType;
        using enum ObjectStatus;
        texinfo = info->editorInfo.skin->get_object_texture(type, COMMON);
        return texinfo;
    }

    // 转化像素位置到谱面坐标系
    MapAxis getPixelMapAxis(const glm::vec2& pixel) const {
        MapAxis axis;
        auto map = info->editorInfo.map;
        auto& beat_timeline = map->beat_timeline();
        auto& beat_info = map->beat_info();
        axis.time = converter->pixelToTime(
            canvas_height - pixel.y - judgeline_absolute_y,
            presentation_canvas_time);
        axis.mousetime = axis.time;
        axis.y =
            converter->timeToPixel(axis.time, presentation_canvas_time, info);
        // 查询最近的分拍
        auto divinfo =
            findNearestDivisorLine(axis.time, beat_timeline, beat_info);
        if (divinfo.is_valid()) {
            // 更新吸附到最近的分拍
            axis.time = divinfo.divisor_time;
            axis.y = converter->timeToPixel(axis.time, presentation_canvas_time,
                                            info);
        } else {
            axis.time = -1;
        }
        // 更新轨道
        for (int i{0}; i < track_count; ++i) {
            if (pixel.x > all_tracks_rect.x + i * single_track_width) {
                axis.track = i;
            }
        }
        // 物件头的x轴位置(中心)
        axis.x =
            all_tracks_rect.x + (float(axis.track) + 0.5f) * single_track_width;
        return axis;
    }

    void generateHeadMesh(const entt::entity& e, GeneratedMesh& entity_mesh,
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

    enum class HoldTailType {
        GENERAL,
        NODE,
    };

    enum class SlideTailType {
        GENERAL,
        NODE,
    };

    void generateHoldMesh(const entt::entity& e, GeneratedMesh& entity_mesh,
                          const uint32_t& time, const float& obj_scale,
                          const float& x, const float& y,
                          HoldTailType tailType =
                              MeshGenerateSystem::HoldTailType::GENERAL) const {
        // 更新物件头部位为更精确的内部位置
        entity_mesh.mesh.back().part = NotePart::HOLD_HEAD;
        // 判断悬浮情况
        auto hovered_entity =
            hovered_info.has_value() && hovered_info.value().source_entity == e;

        // 是否悬浮在面身部分
        auto hovered_hold_body =
            hovered_entity && hovered_info.value().part == NotePart::HOLD_BODY;

        // 计算持续面身高度
        auto [duration] = registry->get<HoldComponent>(e);

        // --------------------面条尾拖动交互--------------------------
        // 判断是否在拖动面尾-更新面条持续时间
        auto drag_info = tool_interaction_state->getDragState();
        if (drag_info.dragged_entities.contains(e) &&
            drag_info.drag_start_hit.part == NotePart::HOLD_END) {
            auto mousePressPos =
                tool_interaction_state->getMouseState().press_pos;
            auto mousePos = tool_interaction_state->getMouseState().current_pos;
            auto end_axis = getPixelMapAxis(mousePos);
            // 验证合法性
            auto validity = end_axis.time >= time;
            // qDebug() << "headtime:" << time;
            // qDebug() << "endtime:" << end_axis.time;

            tool_interaction_state->setDragValidity(validity);
            drag_info = tool_interaction_state->getDragState();
            if (drag_info.is_valid) {
                // 使用实时鼠标位置计算面条持续时间
                duration = end_axis.time - time;
            }
        }

        // 根据面条持续时间计算面身高度
        auto body_height =
            y -
            converter->timeToPixel(
                time + duration,
                info->realTimeInfo.current_time_info.presentation_canvas_time,
                info);

        // 获取面身纹理
        TextureInfo hold_body_texinfo = tex(TexType::HOLD_BODY_VERTICAL);

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
        // 尾部
        switch (tailType) {
            case HoldTailType::GENERAL: {
                generateHoldTailMesh(e, entity_mesh, time, obj_scale, x, y,
                                     body_height);
                break;
            }
            default:
                break;
        }
    }
    void generateHoldTailMesh(const entt::entity& e, GeneratedMesh& entity_mesh,
                              const uint32_t& time, const float& obj_scale,
                              const float& x, const float& y,
                              float& body_height) const {
        // 判断悬浮情况
        auto hovered_entity =
            hovered_info.has_value() && hovered_info.value().source_entity == e;
        // 绘制一个面尾网格(同样画在面条结束的位置)
        // 是否悬浮在面尾部分
        auto hovered_hold_tail =
            hovered_entity && hovered_info.value().part == NotePart::HOLD_END;
        // 面尾网格(在层级1)
        // 获取面尾纹理
        TextureInfo hold_end_texinfo = tex(TexType::HOLD_END);
        auto end_size = hold_end_texinfo.origin_size * obj_scale;
        auto end_pos =
            glm::vec2(x - end_size.x / 2.f, y - body_height - end_size.y / 2.f);
        entity_mesh.mesh.emplace_back(
            end_pos, end_size, hold_end_texinfo, NotePart::HOLD_END,
            hovered_hold_tail ? PartState::GLOW : PartState::NONE, 1);
    }

    void generateMesh(int32_t track_index, const entt::entity& e,
                      GeneratedMesh& entity_mesh, const uint32_t& time,
                      const float& y) const {
        // 根据note信息生成网格

        // 物件头的x轴位置(中心)
        const float x = all_tracks_rect.x +
                        (float(track_index) + 0.5f) * single_track_width;

        // 头的纹理选择
        TextureInfo head_texinfo =
            tex((registry->all_of<HoldComponent>(e) ||
                 registry->all_of<FlickComponent>(e) ||
                 registry->all_of<CompositeRootComponent>(e))
                    ? TexType::HOLD_HEAD
                    : TexType::NORMAL_NOTE);

        // 物件整体缩放计算(统一放大1.25x)
        const auto obj_scale =
            single_track_width / head_texinfo.origin_size.x * 1.25f;

        glm::vec2 head_size = obj_scale * head_texinfo.origin_size;

        // 共同的头网格(head在层级2(最上层))
        auto head_pos = glm::vec2(x - head_size.x / 2.f, y - head_size.y / 2.f);
        generateHeadMesh(e, entity_mesh, x, y, head_pos, head_size,
                         head_texinfo);

        if (registry->all_of<HoldComponent>(e)) {
            generateHoldMesh(e, entity_mesh, time, obj_scale, x, y);
        } else if (registry->all_of<FlickComponent>(e)) {
            // 获取flick身纹理
            // 绘制一个滑尾网格
        } else if (registry->all_of<CompositeRootComponent>(e)) {
            // 组合键-二级遍历
        }
    }
};
#endif  // MMM_MESHGENERATESYSTEM_HPP
