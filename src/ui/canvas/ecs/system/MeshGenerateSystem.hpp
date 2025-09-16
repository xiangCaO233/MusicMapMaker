#ifndef MMM_MESHGENERATESYSTEM_HPP
#define MMM_MESHGENERATESYSTEM_HPP

#include <cstdlib>
#include <ecs/ECSCore.hpp>
#include <ecs/component/CoreComponents.hpp>
#include <ecs/component/NoteComponents.hpp>
#include <ecs/component/RelationComponents.hpp>
#include <ecs/component/TransformComponents.hpp>
#include <ecs/system/TimePixelConverter.hpp>
#include <info/MapCanvasInfo.hpp>
#include <layer/ILayer.hpp>
#include <map/skin/MSkin.hpp>
#include <mmm/map/MMap.hpp>

class MeshGenerateSystem {
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

            // 最终来源实体
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
                    tool_interaction_state->setDragValidity(validity);
                    drag_info = tool_interaction_state->getDragState();
                    if (drag_info.is_valid) {
                        time = axis.time;
                        track_index = axis.track;
                        y = axis.y;
                        tool_interaction_state->setDragValidRes(e, axis);
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

    // 生成头网格
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

    // 生成面条网格
    void generateHoldMesh(const entt::entity& e, GeneratedMesh& entity_mesh,
                          const uint32_t& src_time, const float& obj_scale,
                          const float& src_x, const float& src_y,
                          HoldTailType tailType =
                              MeshGenerateSystem::HoldTailType::GENERAL) const {
        // 更新物件头部位为更精确的内部位置
        entity_mesh.mesh.back().part = NotePart::HOLD_HEAD;
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
        if (drag_info.dragged_entities.contains(e) &&
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

        auto body_width = hold_body_texinfo.origin_size.x * obj_scale;

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
                generateHoldTailMesh(e, entity_mesh, obj_scale, thisx, thisy,
                                     body_height);
                break;
            }
            case HoldTailType::NODE: {
                generateHoldNodeMesh(e, entity_mesh, obj_scale, thisx, thisy,
                                     body_height);
                break;
            }
        }
    }

    // 生成面条尾网格
    void generateHoldTailMesh(const entt::entity& e, GeneratedMesh& entity_mesh,
                              const float& obj_scale, const float& thisx,
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
        auto end_size = hold_end_texinfo.origin_size * obj_scale;
        auto end_pos = glm::vec2(thisx - end_size.x / 2.f,
                                 thisy - body_height - end_size.y / 2.f);
        entity_mesh.mesh.emplace_back(
            end_pos, end_size, hold_end_texinfo, NotePart::HOLD_END,
            hovered_hold_tail ? PartState::GLOW : PartState::NONE, 1);
    }

    // 生成面条尾节点网格
    void generateHoldNodeMesh(const entt::entity& e, GeneratedMesh& entity_mesh,
                              const float& obj_scale, const float& thisx,
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
        auto end_size = node_texinfo.origin_size * obj_scale;
        auto end_pos = glm::vec2(thisx - end_size.x / 2.f,
                                 thisy - body_height - end_size.y / 2.f);
        entity_mesh.mesh.emplace_back(
            end_pos, end_size, node_texinfo, NotePart::HOLD_NODE,
            hovered_hold_node ? PartState::GLOW : PartState::NONE, 1);
    }

    // 生成滑键网格
    void generateSlideMesh(
        const entt::entity& e, GeneratedMesh& entity_mesh,
        const uint32_t& src_track, const float& obj_scale, const float& src_x,
        const float& src_y,
        SlideTailType tailType =
            MeshGenerateSystem::SlideTailType::GENERAL) const {
        // 更新物件头部位为更精确的内部位置
        entity_mesh.mesh.back().part = NotePart::SLIDE_HEAD;
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

    void generateMesh(int32_t track_index, const entt::entity& e,
                      GeneratedMesh& entity_mesh, const uint32_t& time,
                      const float& y, bool child_and_end = false) const {
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

        if (!registry->all_of<ChildOfComponent>(e)) {
            generateHeadMesh(e, entity_mesh, x, y, head_pos, head_size,
                             head_texinfo);
        } else {
            entity_mesh.child_entity = e;
        }

        if (registry->all_of<HoldComponent>(e)) {
            // 检查是否为子物件-区分尾部绘制的是节点还是面尾
            // 默认使用面尾渲染
            auto tail = MeshGenerateSystem::HoldTailType::GENERAL;
            if (registry->all_of<ChildOfComponent>(e)) {
                // 若面条是组合键中的子物件-切换为节点作为面尾渲染
                tail = MeshGenerateSystem::HoldTailType::NODE;
                if (child_and_end) {
                    // 若面条是组合键中的子物件的最后一个-切换为正常面尾渲染
                    tail = MeshGenerateSystem::HoldTailType::GENERAL;
                }
            }
            // 生成面条网格
            generateHoldMesh(e, entity_mesh, time, obj_scale, x, y, tail);
        } else if (registry->all_of<FlickComponent>(e)) {
            auto tail = MeshGenerateSystem::SlideTailType::GENERAL;
            if (registry->all_of<ChildOfComponent>(e)) {
                // 若滑键是组合键中的子物件-切换为节点作为滑尾渲染
                tail = MeshGenerateSystem::SlideTailType::NODE;
                if (child_and_end) {
                    // 若滑键是组合键中的子物件的最后一个-切换为正常滑尾渲染
                    tail = MeshGenerateSystem::SlideTailType::GENERAL;
                }
            }
            // 生成滑键网格
            generateSlideMesh(e, entity_mesh, track_index, obj_scale, x, y,
                              tail);
        } else if (registry->all_of<CompositeRootComponent>(e)) {
            auto& [children] = registry->get<CompositeRootComponent>(e);
            // 组合键-二级遍历
            auto count{0};
            for (const auto& child_e : children) {
                // 获取note原始详细信息
                auto [child_time] = registry->get<TimeComponent>(child_e);
                auto [child_track_index, handle] =
                    registry->get<NoteComponent>(child_e);
                auto [child_y] = registry->get<TransformComponent>(child_e);
                // qDebug() << "生成子物件实体网格:"
                //          << static_cast<uint32_t>(child_e);
                generateMesh(child_track_index, child_e, entity_mesh,
                             child_time, child_y,
                             // 是否为末尾
                             ++count == children.size());
                ;
            }
            // qDebug() << "共" << children.size() << "个子实体";
        }
    }
};
#endif  // MMM_MESHGENERATESYSTEM_HPP
