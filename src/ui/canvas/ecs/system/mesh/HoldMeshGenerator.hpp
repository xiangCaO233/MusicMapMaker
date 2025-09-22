#ifndef MMM_HOLDMESHGENERATOR_HPP
#define MMM_HOLDMESHGENERATOR_HPP

#include <ecs/component/CoreComponents.hpp>
#include <ecs/component/NoteComponents.hpp>
#include <ecs/component/RelationComponents.hpp>
#include <ecs/component/TransformComponents.hpp>
#include <ecs/system/TimePixelConverter.hpp>
#include <entt.hpp>
#include <info/MapCanvasInfo.hpp>
#include <map/skin/MSkin.hpp>
#include <mmm/map/MMap.hpp>

class HoldMeshGenerator {
   public:
    HoldMeshGenerator(const glm::vec4& all_tracks_rect, int track_count,
                      float judgeline_absolute_y, float single_track_width,
                      float canvas_height, double presentation_canvas_time,
                      std::optional<MeshPartInfo> hovered_info,
                      MapCanvasInfo* info,
                      ToolInteractionState* tool_interaction_state,
                      const TimePixelConverter* converter,
                      const entt::registry* registry)
        : all_tracks_rect(all_tracks_rect),
          track_count(track_count),
          judgeline_absolute_y(judgeline_absolute_y),
          single_track_width(single_track_width),
          canvas_height(canvas_height),
          presentation_canvas_time(presentation_canvas_time),
          hovered_info(hovered_info),
          info(info),
          tool_interaction_state(tool_interaction_state),
          converter(converter),
          registry(registry) {};
    glm::vec4 all_tracks_rect;
    int track_count;
    float judgeline_absolute_y;
    float single_track_width;
    float canvas_height;
    double presentation_canvas_time;
    std::optional<MeshPartInfo> hovered_info;
    MapCanvasInfo* info;
    ToolInteractionState* tool_interaction_state;
    const TimePixelConverter* converter;
    const entt::registry* registry;
    enum class HoldTailType {
        GENERAL,
        NODE,
    };
    // 获取纹理
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

    // 生成面条网格
    void generateHoldMesh(const entt::entity& e, GeneratedMesh& entity_mesh,
                          const uint32_t& src_time,
                          const float& obj_scale_width,
                          const float& obj_scale_height, const float& src_x,
                          const float& src_y, HoldTailType tailType) const {
        if (!entity_mesh.mesh.empty()) {
            // 更新物件头部位为更精确的内部位置
            entity_mesh.mesh.back().part = NotePart::HOLD_HEAD;
        }
        // 判断悬浮情况
        auto hovered_entity = hovered_info.has_value() &&
                              (hovered_info.value().source_entity == e ||
                               hovered_info.value().child_entity == e);

        // 是否悬浮在面身部分
        auto hovered_hold_body =
            hovered_entity && hovered_info.value().part == NotePart::HOLD_BODY;

        // 计算持续面身高度
        auto [duration] = registry->get<HoldComponent>(e);
        // 有可能属于组合物件中,组合物件的位置内的面条时间/y/x/轨道需要更新为当前子物件的
        auto thistime = tailType == HoldTailType::GENERAL
                            ? src_time
                            : registry->get<TimeComponent>(e).timestamp;

        auto thisy = tailType == HoldTailType::GENERAL
                         ? src_y
                         : registry->get<TransformComponent>(e).y;
        auto thisx =
            tailType == HoldTailType::GENERAL
                ? src_x
                : all_tracks_rect.x +
                      (float(registry->get<NoteComponent>(e).track_index) +
                       0.5f) *
                          single_track_width;

        // --------------------面条尾拖动交互--------------------------
        // 判断是否在拖动单纯面条的面尾-更新面条持续时间
        auto drag_info = tool_interaction_state->getDragState();
        if (drag_info.dragged_entitiesWithRes.contains(e) &&
            drag_info.drag_start_hit.part == NotePart::HOLD_END) {
            auto mousePressPos =
                tool_interaction_state->getMouseState().press_pos;
            auto mousePos = tool_interaction_state->getMouseState().current_pos;
            auto end_axis = getPixelMapAxis(mousePos);
            // 验证合法性
            auto validity = end_axis.time >= thistime;
            // qDebug() << "headtime:" << time;
            // qDebug() << "endtime:" << end_axis.time;

            tool_interaction_state->setDragValidity(validity);
            drag_info = tool_interaction_state->getDragState();
            if (drag_info.is_valid) {
                // 使用实时鼠标位置计算面条持续时间
                duration = end_axis.time - thistime;
                tool_interaction_state->setDragValidRes(e, end_axis);
            }
        }

        // 根据面条持续时间计算面身高度
        auto body_height =
            thisy -
            converter->timeToPixel(
                thistime + duration,
                info->realTimeInfo.current_time_info.presentation_canvas_time,
                info);

        // 获取面身纹理
        TextureInfo hold_body_texinfo = tex(TexType::HOLD_BODY_VERTICAL);

        auto body_width = hold_body_texinfo.origin_size.x * obj_scale_width;

        // 绘制面身(画在面条结束的位置)
        // 面身网格(在层级0(最下层))
        auto body_pos =
            glm::vec2(thisx - body_width / 2.f, thisy - body_height);
        auto body_size = glm::vec2(body_width, body_height);

        entity_mesh.mesh.emplace_back(
            body_pos, body_size, hold_body_texinfo, NotePart::HOLD_BODY,
            hovered_hold_body ? PartState::GLOW : PartState::NONE, 0,
            // 面身为重复绘制纹理模式(基于给定宽)
            TexScaleMode::TILE_BASEWIDTH_REPEAT);

        // 尾部
        switch (tailType) {
            case HoldTailType::GENERAL: {
                generateHoldTailMesh(e, entity_mesh, obj_scale_width,
                                     obj_scale_height, thisx, thisy,
                                     body_height);
                break;
            }
            case HoldTailType::NODE: {
                generateHoldNodeMesh(e, entity_mesh, obj_scale_width,
                                     obj_scale_height, thisx, thisy,
                                     body_height);
                break;
            }
        }
    }

    // 生成面条尾网格
    void generateHoldTailMesh(const entt::entity& e, GeneratedMesh& entity_mesh,
                              const float& obj_scale_width,
                              const float& obj_scale_height, const float& thisx,
                              const float& thisy, float& body_height) const {
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
        auto end_size =
            glm::vec2{hold_end_texinfo.origin_size.x * obj_scale_width,
                      hold_end_texinfo.origin_size.y * obj_scale_height};
        auto end_pos = glm::vec2(thisx - end_size.x / 2.f,
                                 thisy - body_height - end_size.y / 2.f);
        entity_mesh.mesh.emplace_back(
            end_pos, end_size, hold_end_texinfo, NotePart::HOLD_END,
            hovered_hold_tail ? PartState::GLOW : PartState::NONE, 1);
    }

    // 生成面条尾节点网格
    void generateHoldNodeMesh(const entt::entity& e, GeneratedMesh& entity_mesh,
                              const float& obj_scale_width,
                              const float& obj_scale_height, const float& thisx,
                              const float& thisy, float& body_height) const {
        // 判断悬浮情况(仅可能为子实体)
        auto hovered_entity =
            hovered_info.has_value() && hovered_info.value().child_entity == e;
        // 绘制一个面尾网格(同样画在面条结束的位置)
        // 是否悬浮在面尾节点部分
        auto hovered_hold_node =
            hovered_entity && hovered_info.value().part == NotePart::HOLD_NODE;
        // 面尾网格(在层级1)
        // 获取面尾纹理
        TextureInfo node_texinfo = tex(TexType::NODE);
        auto end_size =
            glm::vec2{node_texinfo.origin_size.x * obj_scale_width,
                      node_texinfo.origin_size.y * obj_scale_height};
        auto end_pos = glm::vec2(thisx - end_size.x / 2.f,
                                 thisy - body_height - end_size.y / 2.f);
        entity_mesh.mesh.emplace_back(
            end_pos, end_size, node_texinfo, NotePart::HOLD_NODE,
            hovered_hold_node ? PartState::GLOW : PartState::NONE, 1);
    }
};

#endif  // MMM_HOLDMESHGENERATOR_HPP
