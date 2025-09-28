#include <ecs/system/sync/SyncSystem.hpp>
#include <ecs/system/sync/ToolCommandProcessor.hpp>
#include <info/NotePart.hpp>
// #include <iostream>
#include <log/colorful-log.h>

#include <ecs/component/ComponentInspector.hpp>
#include <layer/MapLayerManager.hpp>
#include <mmm/map/MMap.hpp>
#include <tool/ThreadSafeQueue.hpp>
#include <tool/ToolInteractionState.hpp>
#include <util/mutil.hpp>

void updateHover(ToolSystem* toolSystem,
                 ToolInteractionState* toolInteractionState,
                 const MapCanvasInfo* info) {
    // 获取最新的鼠标位置状态 (由UI线程持续更新)
    MouseState mouse = toolInteractionState->getMouseState();
    glm::vec2 current_mouse_pos = mouse.current_pos;
    auto& main_track_layout = info->editorInfo.track_layout;
    // 更新鼠标的区域信息
    auto xpos = main_track_layout.x + main_track_layout.z;
    auto preview_tracks_rect =
        glm::vec4{xpos, 0, info->baseInfo.canvasSize.width() - xpos,
                  info->baseInfo.canvasSize.height()};
    if (mutil::checkPointInRect(current_mouse_pos, preview_tracks_rect)) {
        toolInteractionState->setMouseArea(MouseArea::PREVIEW);
    } else if (mutil::checkPointInRect(current_mouse_pos, main_track_layout)) {
        toolInteractionState->setMouseArea(MouseArea::EDIT);
    }
    auto& notes = info->editorInfo.map->note_set();
    auto& uuidManager = info->editorInfo.map->note_uuids();
    // 使用 ToolSystem (四叉树) 在当前世界状态下重新查询
    // tool_system 的四叉树是在上一帧的 tick() 中更新的
    // 包含所有物体移动后的最新位置
    auto current_hit = toolSystem->query(current_mouse_pos);

    // 获取旧悬浮状态
    auto old_hover = toolInteractionState->getHover();

    // 进行比较和更新
    if (current_hit.has_value()) {
        // 当前鼠标下有物体
        const auto hit_info = current_hit.value();
        auto& registry = toolSystem->get_registry();
        auto note = notes.get_note(uuidManager.get_handle(hit_info.uuid));
        if (!note) {
            if (hit_info.source_entity != entt::null &&
                hit_info.child_entity == entt::null &&
                hit_info.uuid == InvalidNoteUUID) {
                // 仅sourceentity有效

            } else {
                if (registry.valid(hit_info.child_entity)) {
                    // 组合键子键
                    auto& [parent, child_index] =
                        toolSystem->get_registry().get<ChildOfComponent>(
                            hit_info.child_entity);
                    // 父实体失效
                    if (!toolSystem->get_registry().valid(parent)) return;
                    auto& [parent_track, parent_uuid] =
                        toolSystem->get_registry().get<NoteComponent>(parent);
                    auto parent_note =
                        notes.get_note(uuidManager.get_handle(parent_uuid));
                    if (!parent_note) {
                        // 父实体失效
                        return;
                    } else {
                        // 获取到此子物件
                        note = static_cast<const Composite*>(parent_note)
                                   ->children()[child_index]
                                   .get();
                    }
                }
            }
        }

        // 检查是否和旧的悬浮状态是同一个物体/部位
        if (!old_hover.has_value() ||
            old_hover.value().source_entity != hit_info.source_entity ||
            old_hover.value().child_entity != hit_info.child_entity ||
            old_hover.value().part != hit_info.part) {
            // 不一样，或者之前没有悬浮 -> 更新为新的悬浮状态
            toolInteractionState->setHover(hit_info);

            // toolSystem->get_mesh_info_tree().print_tree();

            XINFO("source_entity:" + std::to_string(static_cast<uint32_t>(
                                         hit_info.source_entity)));
            auto ins = ComponentInspector::inspect<TimeComponent, NoteComponent,
                                                   TransformComponent>(
                toolSystem->get_registry(), hit_info.source_entity);
            XINFO(ins);
            if (toolSystem->get_registry().valid(hit_info.child_entity)) {
                XINFO("child_entity:" + std::to_string(static_cast<uint32_t>(
                                            hit_info.child_entity)));
                ins = ComponentInspector::inspect<TimeComponent, NoteComponent,
                                                  TransformComponent>(
                    toolSystem->get_registry(), hit_info.child_entity);
                XINFO(ins);
            }
        }

        // qDebug() << "当前hover到id:" << hit_info.handle.index;
        // qDebug() << "当前hover到" << note->toString();
        // qDebug() << "part:" << to_string(hit_info.part);

        // 如果一样，什么都不做，保持状态
    } else {
        // 当前鼠标下没有物体
        if (old_hover.has_value()) {
            // 但之前有悬浮 -> 清除悬浮状态
            toolInteractionState->setHover(std::nullopt);
            // qDebug() << "鼠标下无物体,清理悬浮状态";
        }
        // 如果之前也没有，就什么都不做
    }
}

// 将像素X坐标转换为轨道索引
int pixelToTrackIndex(float pixel_x, const glm::vec4& all_tracks_rect,
                      int track_count) {
    if (track_count == 0) return -1;
    // 检查是否在轨道区域外
    if (pixel_x < all_tracks_rect.x) {
        return 0;
    }
    if (pixel_x > all_tracks_rect.x + all_tracks_rect.z) {
        return track_count - 1;
    }
    // 计算相对位置并转换为索引
    float relative_x = pixel_x - all_tracks_rect.x;
    float single_track_width = all_tracks_rect.z / track_count;
    return static_cast<int>(relative_x / single_track_width);
}

void updateSelections(entt::registry& registry, ToolSystem* toolSystem,
                      ToolInteractionState* toolInteractionState,
                      const MapCanvasInfo* info) {
    auto selectState = toolInteractionState->getSelectionState();
    auto& left_select_time_areas = selectState.per_button_areas[Qt::LeftButton];

    // 最终所有被选中的实体的集合
    std::unordered_set<entt::entity> selected_entities{};

    // 如果没有选择框，则清空选择并直接返回
    if (left_select_time_areas.empty()) {
        toolInteractionState->setSelection(Qt::LeftButton, selected_entities);
        return;
    }

    auto& all_tracks_rect = info->editorInfo.track_layout;
    auto track_count = info->editorInfo.map->base_metadata().track_count;
    if (track_count == 0) return;

    // 当前帧画布的逻辑时间，用于坐标转换
    const int64_t current_canvas_time =
        info->realTimeInfo.current_time_info.presentation_canvas_time;

    // 将所有选择框的逻辑区域预先计算好
    struct LogicalArea {
        int64_t start_time;
        int64_t end_time;
        int start_track;
        int end_track;
    };
    std::vector<LogicalArea> logical_selection_areas;

    for (const auto& time_area : left_select_time_areas) {
        // 转换时间范围
        int64_t time1 = time_area.y;
        int64_t time2 = time1 + static_cast<int64_t>(time_area.w);

        // 转换轨道范围
        int track1 =
            pixelToTrackIndex(time_area.x, all_tracks_rect, track_count);
        int track2 = pixelToTrackIndex(time_area.x + time_area.z,
                                       all_tracks_rect, track_count);

        // qDebug() << "area track[" << track1 << "]to[" << track2 << "]";
        // qDebug() << "area time[" << time1 << "]to[" << time2 << "]";

        // 检查轨道索引是否有效
        if (track1 < 0 || track2 < 0) continue;

        // 规范化范围，确保 start <= end
        logical_selection_areas.push_back(
            {std::min(time1, time2), std::max(time1, time2),
             std::min(track1, track2), std::max(track1, track2)});
    }

    // 遍历所有 Note 实体，进行碰撞检测
    auto view = registry.view<const NoteComponent, const TimeComponent>();
    for (auto entity : view) {
        const auto& note = registry.get<NoteComponent>(entity);
        const auto& time = registry.get<TimeComponent>(entity);

        // 检查这个 Note 是否落在任何一个逻辑选择框内
        for (const auto& area : logical_selection_areas) {
            bool time_overlaps = (time.timestamp >= area.start_time &&
                                  time.timestamp <= area.end_time);
            bool track_overlaps = (note.track_index >= area.start_track &&
                                   note.track_index <= area.end_track);

            if (time_overlaps && track_overlaps) {
                if (registry.all_of<ChildOfComponent>(entity)) {
                    auto& [parent, index] =
                        registry.get<ChildOfComponent>(entity);
                    if (!selected_entities.contains(parent)) {
                        selected_entities.insert(parent);
                    }
                } else {
                    if (!selected_entities.contains(entity)) {
                        selected_entities.insert(entity);
                    }
                }
                // 一旦被选中，就无需再检查其他选择框了
                break;
            }
        }
    }

    // 更新最终的选择状态
    toolInteractionState->setSelection(Qt::LeftButton, selected_entities);
}

// 处理所有工具指令
void processToolCommands(entt::registry& registry,
                         ThreadSafeQueue<ToolCommand>* toolCmdQ,
                         ToolSystem* system, std::shared_ptr<MMapEditor> editor,
                         ToolInteractionState* toolInteractionState, MMap* map,
                         MapCanvasInfo* info,
                         const TimePixelConverter& maintrack_converter,
                         const TimePixelConverter& preview_converter) {
    auto cmds = toolCmdQ->drain();
    if (cmds.empty()) return;

    for (const auto& command : cmds) {
        ToolCommandProcessor processor(
            registry, *system, *editor, *toolInteractionState, map, info,
            &maintrack_converter, &preview_converter);
        processor.process(command);
    }
}

// 同步工具交互
void SyncSystem::updateToolInteractions(
    ECSCore& core, MapCanvasInfo* info, MapLayerManager* layer_manager,
    const TimePixelConverter& maintrack_converter,
    const TimePixelConverter& preview_converter) const {
    // qDebug() << "同步系统->同步工具状态(at pretick)开始";
    auto toolSystem = layer_manager->get_tool_system();
    auto toolCmdQ = layer_manager->get_tool_cmdq();
    auto toolInteractionState = layer_manager->get_tool_interaction_state();

    // qDebug() << "同步系统->同步工具状态->处理工具指令(at pretick)开始";
    // 更新悬浮状态
    auto editor = info->editorInfo.map->editor();
    processToolCommands(core.ecs_registry(), toolCmdQ, toolSystem, editor,
                        toolInteractionState, info->editorInfo.map, info,
                        maintrack_converter, preview_converter);
    // qDebug() << "同步系统->同步工具状态->处理工具指令(at pretick)结束";

    // 更新选中内容
    updateSelections(core.ecs_registry(), toolSystem, toolInteractionState,
                     info);

    // qDebug() << "同步系统->同步工具状态->处理实时悬浮检测(at pretick)开始";
    updateHover(toolSystem, toolInteractionState, info);
    // qDebug() << "同步系统->同步工具状态->处理实时悬浮检测(at pretick)结束";

    // qDebug() << "同步系统->同步工具状态(at pretick)结束";
}
