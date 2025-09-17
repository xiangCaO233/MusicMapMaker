#ifndef MMM_SLIDEMESHGENERATOR_HPP
#define MMM_SLIDEMESHGENERATOR_HPP

#include <ecs/component/CoreComponents.hpp>
#include <ecs/component/NoteComponents.hpp>
#include <ecs/component/RelationComponents.hpp>
#include <ecs/component/TransformComponents.hpp>
#include <ecs/system/TimePixelConverter.hpp>
#include <entt.hpp>
#include <info/MapCanvasInfo.hpp>
#include <map/skin/MSkin.hpp>
#include <mmm/map/MMap.hpp>

class SlideMeshGenerator {
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

   public:
    SlideMeshGenerator(const glm::vec4& all_tracks_rect, int track_count,
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
    enum class SlideTailType {
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
    DragState::MapAxis getPixelMapAxis(const glm::vec2& pixel) const {
        DragState::MapAxis axis;
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

    // 生成滑键网格
    void generateSlideMesh(const entt::entity& e, GeneratedMesh& entity_mesh,
                           const uint32_t& src_track, const float& obj_scale,
                           const float& src_x, const float& src_y,
                           SlideTailType tailType) const {
        if (!entity_mesh.mesh.empty()) {
            // 更新物件头部位为更精确的内部位置
            entity_mesh.mesh.back().part = NotePart::SLIDE_HEAD;
        }
        // 判断悬浮情况
        auto hovered_entity = hovered_info.has_value() &&
                              (hovered_info.value().source_entity == e ||
                               hovered_info.value().child_entity == e);
        // 是否悬浮在滑身部分
        auto hovered_hold_body =
            hovered_entity && hovered_info.value().part == NotePart::SLIDE_BODY;
        // 计算滑动轨道差值
        auto [delta_track] = registry->get<FlickComponent>(e);
        // qDebug() << "this slide dtrack:" << delta_track;

        // 有可能属于组合物件中,组合物件的位置内的滑键轨道/y/x需要更新为当前子物件的
        auto thistrack = tailType == SlideTailType::GENERAL
                             ? src_track
                             : registry->get<NoteComponent>(e).track_index;
        auto thisy = tailType == SlideTailType::GENERAL
                         ? src_y
                         : registry->get<TransformComponent>(e).y;
        auto thisx =
            tailType == SlideTailType::GENERAL
                ? src_x
                : all_tracks_rect.x +
                      (float(registry->get<NoteComponent>(e).track_index) +
                       0.5f) *
                          single_track_width;

        // --------------------滑尾拖动交互--------------------------
        // 判断是否在拖动滑动尾-更新滑键轨道差值
        auto drag_info = tool_interaction_state->getDragState();
        if (drag_info.dragged_entities.contains(e) &&
            drag_info.drag_start_hit.part == NotePart::SLIDE_END) {
            auto mousePressPos =
                tool_interaction_state->getMouseState().press_pos;
            auto mousePos = tool_interaction_state->getMouseState().current_pos;
            auto end_axis = getPixelMapAxis(mousePos);
            // 验证合法性
            auto validity =
                end_axis.track <
                    info->editorInfo.map->base_metadata().track_count &&
                end_axis.track != thistrack;
            // qDebug() << "headtime:" << time;
            // qDebug() << "endtime:" << end_axis.time;

            tool_interaction_state->setDragValidity(validity);
            drag_info = tool_interaction_state->getDragState();
            if (drag_info.is_valid) {
                // 使用实时鼠标位置计算面条持续时间
                delta_track = end_axis.track - thistrack;
                tool_interaction_state->setDragValidRes(e, end_axis);
            }
        }

        // 根据滑动轨道差值计算滑身长度
        auto body_width = std::abs(delta_track * single_track_width);

        // 获取滑身纹理
        TextureInfo slide_body_texinfo = tex(TexType::HOLD_BODY_HORIZONTAL);

        auto body_height = slide_body_texinfo.origin_size.y * obj_scale;

        // 绘制滑身(画在滑键的位置或(若delta为负数画左向width处))
        // 面身网格(在层级0(最下层))
        auto delta_width = delta_track < 0 ? -body_width : body_width;
        auto body_pos = glm::vec2(thisx + (delta_width > 0 ? 0 : delta_width),
                                  thisy - body_height / 2.f);
        auto body_size = glm::vec2(body_width, body_height);

        entity_mesh.mesh.emplace_back(
            body_pos, body_size, slide_body_texinfo, NotePart::SLIDE_BODY,
            hovered_hold_body ? PartState::GLOW : PartState::NONE, 0,
            // 面身为重复绘制纹理模式(基于给定宽)
            TexScaleMode::TILE_BASEHEIGHT_REPEAT);

        // 尾部
        switch (tailType) {
            case SlideTailType::GENERAL: {
                generateSlideTailMesh(e, entity_mesh, thistrack, delta_width,
                                      obj_scale, thisx, thisy, body_height);
                break;
            }
            case SlideTailType::NODE: {
                generateSlideNodeMesh(e, entity_mesh, src_track, delta_width,
                                      obj_scale, thisx, thisy, body_height);
                break;
            }
        }
    }

    // 生成滑键尾部网格
    void generateSlideTailMesh(const entt::entity& e,
                               GeneratedMesh& entity_mesh,
                               const uint32_t& srctrack,
                               const float& delta_width, const float& obj_scale,
                               const float& thisx, const float& thisy,
                               float& body_height) const {
        // 判断悬浮情况
        auto hovered_entity =
            hovered_info.has_value() && hovered_info.value().source_entity == e;
        // 绘制一个滑尾网格(同样画在面条结束的位置)
        // 是否悬浮在滑尾部分
        auto hovered_slide_tail =
            hovered_entity && hovered_info.value().part == NotePart::SLIDE_END;
        // 滑尾网格(在层级1)
        // 获取滑尾纹理
        TextureInfo slide_end_texinfo =
            tex(delta_width < 0 ? TexType::SLIDE_END_LEFT
                                : TexType::SLIDE_END_RIGHT);
        auto end_size = slide_end_texinfo.origin_size * obj_scale;
        auto end_pos = glm::vec2(thisx - end_size.x / 2.f + delta_width,
                                 thisy - end_size.y / 2.f);
        entity_mesh.mesh.emplace_back(
            end_pos, end_size, slide_end_texinfo, NotePart::SLIDE_END,
            hovered_slide_tail ? PartState::GLOW : PartState::NONE, 1);
    }

    // 生成滑键尾节点网格
    void generateSlideNodeMesh(const entt::entity& e,
                               GeneratedMesh& entity_mesh,
                               const uint32_t& srctrack,
                               const float& delta_width, const float& obj_scale,
                               const float& thisx, const float& thisy,
                               float& body_height) const {
        // 判断悬浮情况
        auto hovered_entity = hovered_info.has_value() &&
                              (hovered_info.value().source_entity == e ||
                               hovered_info.value().child_entity == e);
        // 绘制一个滑尾网格(同样画在面条结束的位置)
        // 是否悬浮在节点部分
        auto hovered_slide_node =
            hovered_entity && hovered_info.value().part == NotePart::SLIDE_NODE;
        // 滑尾节点网格(在层级1)
        // 获取节点纹理
        TextureInfo node_texinfo = tex(TexType::NODE);
        auto end_size = node_texinfo.origin_size * obj_scale;
        auto end_pos = glm::vec2(thisx - end_size.x / 2.f + delta_width,
                                 thisy - end_size.y / 2.f);
        entity_mesh.mesh.emplace_back(
            end_pos, end_size, node_texinfo, NotePart::SLIDE_NODE,
            hovered_slide_node ? PartState::GLOW : PartState::NONE, 1);
    }
};

#endif  // MMM_SLIDEMESHGENERATOR_HPP
