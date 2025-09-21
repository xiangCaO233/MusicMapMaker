#ifndef MMM_MESHGENERATESYSTEM_HPP
#define MMM_MESHGENERATESYSTEM_HPP

#include <ecs/ECSCore.hpp>
#include <ecs/component/CoreComponents.hpp>
#include <ecs/component/NoteComponents.hpp>
#include <ecs/component/RelationComponents.hpp>
#include <ecs/component/TransformComponents.hpp>
#include <ecs/system/TimePixelConverter.hpp>
#include <ecs/system/mesh/HoldMeshGenerator.hpp>
#include <ecs/system/mesh/SlideMeshGenerator.hpp>
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
    std::unordered_map<entt::entity, GeneratedMesh>* generated_meshes;

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
    // 获取纹理
    TextureInfo tex(TexType type) const {
        TextureInfo texinfo;
        using enum TexType;
        using enum ObjectStatus;
        texinfo = info->editorInfo.skin->get_object_texture(type, COMMON);
        return texinfo;
    }

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
        generated_meshes = &out_generated_meshes;

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
            auto [track_index, uuid] = registry->get<NoteComponent>(e);
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
                auto mouseState = tool_interaction_state->getMouseState();
                auto& dragpart = drag_info.drag_start_hit.part;
                auto& mousePressPos = mouseState.press_pos;
                auto& mousePos = mouseState.current_pos;
                // auto mouse_time =
                //     converter.pixelToTime(info->baseInfo.canvasSize.height()
                //     -
                //                               mousePos.y -
                //                               judgeline_absolute_y,
                //                           info->realTimeInfo.current_time_info
                //                               .presentation_canvas_time);
                if (dragpart == NotePart::HEAD ||
                    dragpart == NotePart::HOLD_HEAD ||
                    dragpart == NotePart::SLIDE_HEAD) {
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
                // 非虚影或即将删除方式渲染
                // 区分是否为即将创建
                if (uuid == InvalidNoteUUID) {
                    // 无uuid,是即将创建的物件-跟随创建状态中的创建节点
                    // 节点需在同步系统中完成InvalidNoteUUID的实体创建和附件更新
                    // 使用虚影渲染
                    entity_mesh.state = MeshState::GHOST;
                } else {
                    // 有uuid,是真实在谱面中存在的物件-正常渲染
                }
                generateMesh(track_index, e, entity_mesh, time, y);
            }
        }
    }

   private:
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
        }

        if (registry->all_of<HoldComponent>(e)) {
            HoldMeshGenerator holdMeshGenerator(
                all_tracks_rect, track_count, judgeline_absolute_y,
                single_track_width, canvas_height, presentation_canvas_time,
                hovered_info, info, tool_interaction_state, converter,
                registry);
            // 检查是否为子物件-区分尾部绘制的是节点还是面尾
            // 默认使用面尾渲染
            auto tail = HoldMeshGenerator::HoldTailType::GENERAL;
            if (registry->all_of<ChildOfComponent>(e)) {
                // 若面条是组合键中的子物件-切换为节点作为面尾渲染
                tail = HoldMeshGenerator::HoldTailType::NODE;
                if (child_and_end) {
                    // 若面条是组合键中的子物件的最后一个-切换为正常面尾渲染
                    tail = HoldMeshGenerator::HoldTailType::GENERAL;
                }
            }
            // 生成面条网格
            holdMeshGenerator.generateHoldMesh(e, entity_mesh, time, obj_scale,
                                               x, y, tail);
        } else if (registry->all_of<FlickComponent>(e)) {
            SlideMeshGenerator slideMeshGenerator(
                all_tracks_rect, track_count, judgeline_absolute_y,
                single_track_width, canvas_height, presentation_canvas_time,
                hovered_info, info, tool_interaction_state, converter,
                registry);
            auto tail = SlideMeshGenerator::SlideTailType::GENERAL;
            if (registry->all_of<ChildOfComponent>(e)) {
                // 若滑键是组合键中的子物件-切换为节点作为滑尾渲染
                tail = SlideMeshGenerator::SlideTailType::NODE;
                if (child_and_end) {
                    // 若滑键是组合键中的子物件的最后一个-切换为正常滑尾渲染
                    tail = SlideMeshGenerator::SlideTailType::GENERAL;
                }
            }
            // 生成滑键网格
            slideMeshGenerator.generateSlideMesh(e, entity_mesh, track_index,
                                                 obj_scale, x, y, tail);
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
                // qDebug() << "父物件实体:" << static_cast<uint32_t>(e)
                //          << "递归生成子物件实体网格:"
                //          << static_cast<uint32_t>(child_e);
                // 填充目标网格属性
                auto& child_mesh = (*generated_meshes)[child_e];

                // 最终来源实体
                child_mesh.source_entity = e;
                child_mesh.child_entity = child_e;
                // 跟随父物件的网格状态(如虚影)
                child_mesh.state = entity_mesh.state;
                generateMesh(child_track_index, child_e, child_mesh, child_time,
                             child_y,
                             // 是否为末尾
                             ++count == children.size());
            }
            // qDebug() << "共" << children.size() << "个子实体";
        }
    }
};
#endif  // MMM_MESHGENERATESYSTEM_HPP
