#ifndef MMM_TOOLCOMMANDPROCESSOR_HPP
#define MMM_TOOLCOMMANDPROCESSOR_HPP

#include <colorful-log.h>
#include <qforeach.h>

#include <ecs/component/CoreComponents.hpp>
#include <ecs/component/RelationComponents.hpp>
#include <ecs/system/ToolSystem.hpp>
#include <ecs/system/time2pixel/TimePixelConverter.hpp>
#include <info/NotePart.hpp>
#include <mmm/map/editor/MMapEditor.hpp>
#include <mmm/project/AudioLoadCallback.hpp>
#include <tool/ToolInteractionState.hpp>
#include <tool/command/ToolCommand.hpp>
#include <unordered_set>
#include <vector>

// --- Helper for std::visit ---
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
// C++17 class template argument deduction
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

class ToolCommandProcessor {
   public:
    ToolCommandProcessor(entt::registry& r, ToolSystem& s, MMapEditor& e,
                         ToolInteractionState& i, MMap* m, MapCanvasInfo* info,
                         const TimePixelConverter* maintrack_converter,
                         const TimePixelConverter* preview_converter)
        : registry(r),
          system(s),
          interactionState(i),
          mapEditor(e),
          map(m),
          maintrack_converter(maintrack_converter),
          preview_converter(preview_converter),
          info(info) {
        all_tracks_rect = info->editorInfo.track_layout;
        track_count = info->editorInfo.map->base_metadata().track_count;
        single_track_width = all_tracks_rect.z / float(track_count);
        canvas_height = info->baseInfo.canvasSize.height();
        auto pconfig = map->project()->cfg();
        judgeline_absolute_y =
            canvas_height * (1.f - pconfig->canvas_config.judgeline_pos);
        presentation_canvas_time =
            info->realTimeInfo.current_time_info.presentation_canvas_time;
        const auto& editor_info = info->editorInfo;
        const auto& base_info = info->baseInfo;
        const float canvas_height = base_info.canvasSize.height();

        // 预览区相对主轨道的倍率
        auto maintrackpos_inpreview_area_ratio =
            editor_info.previewAreaInfo.areaRatio;
        // 主轨道在预览区中的高度
        auto maintrack_size_inpreview =
            canvas_height / maintrackpos_inpreview_area_ratio;
        // 主轨道中心在预览区中的倍率
        auto maintrackpos_inpreview_area =
            editor_info.previewAreaInfo.mainAreaPos;
        // 主轨道中心在预览区中的位置
        auto maintrack_center_inpreview =
            maintrackpos_inpreview_area * canvas_height;
        // 主轨道顶部在预览区中的位置
        auto maintrack_top_inpreview =
            maintrack_center_inpreview - maintrack_size_inpreview / 2.f;
        // 主轨道判定线在预览区中的位置
        judgeline_pos_in_previewarea =
            maintrack_top_inpreview +
            pconfig->canvas_config.judgeline_pos * maintrack_size_inpreview;
    }

    ~ToolCommandProcessor() = default;

    void process(const ToolCommand& command) {
        std::visit(
            overloaded{
                // 预览相关
                // 开始拖动预览区
                [&](const StartDragPreviewCommand& arg) {
                    if (arg.start_button == Qt::LeftButton) {
                        auto time = preview_converter->distanceToTime(
                            judgeline_pos_in_previewarea -
                                arg.common_info.start_mouse_pos.y,
                            presentation_canvas_time);
                        // 先直接跳转
                        info->realTimeInfo.current_time_info =
                            time -
                            info->realTimeInfo.offset_info
                                .global_static_offset_ms -
                            info->realTimeInfo.offset_info.global_offset_ms;
                        // 更新音频位置
                        info->audio_callback->set_playpos_for(
                            map->base_metadata()
                                .main_audio_path.generic_string(),
                            std::chrono::milliseconds(
                                info->realTimeInfo.current_time_info
                                    .raw_audio_time_ms));
                    } else if (arg.start_button == Qt::RightButton) {
                        // 开始宏观拖动
                        interactionState.startGlobalDragPreview();
                    }
                },
                // 更新拖动预览区位置
                [&](const DragPreviewUpdateCommand& arg) {
                    //
                    if (arg.start_button == Qt::LeftButton) {
                        // 更改预览区内主轨道位置并更新当前时间戳
                        // 获取当前鼠标y位置在预览区对应时间
                        // auto time = preview_converter->distanceToTime(
                        //     judgeline_pos_in_previewarea -
                        //     arg.dragging_pos.y, presentation_canvas_time);
                        // XINFO("预览区内判定线像素y位置:" +
                        //       std::to_string(judgeline_pos_in_previewarea));
                        // XINFO("当前拖拽预览鼠标像素y位置:" +
                        //       std::to_string(arg.dragging_pos.y));
                        // XINFO("拖拽预览转换的时间位置:" +
                        // std::to_string(time));

                        // 获取当前鼠标y位置在预览区对应比例位置
                        // auto height_ratio = arg.dragging_pos.y /
                        // canvas_height;
                        // info->editorInfo.previewAreaInfo.mainAreaPos =
                        //     height_ratio;

                    } else if (arg.start_button == Qt::RightButton) {
                        if (interactionState.isDraggingGlobalPreview()) {
                            float ratio =
                                1.f - (arg.dragging_pos.y / canvas_height);
                            auto time = map->base_metadata().map_length * ratio;
                            // 先直接跳转
                            info->realTimeInfo.current_time_info =
                                time -
                                info->realTimeInfo.offset_info
                                    .global_static_offset_ms -
                                info->realTimeInfo.offset_info.global_offset_ms;
                            // 更新音频位置
                            info->audio_callback->set_playpos_for(
                                map->base_metadata()
                                    .main_audio_path.generic_string(),
                                std::chrono::milliseconds(
                                    info->realTimeInfo.current_time_info
                                        .raw_audio_time_ms));
                        }
                        // 仅更新整体当前时间戳
                    }
                },
                // 结束拖动预览区
                [&](const EndDragPreviewCommand& arg) {
                    //
                    if (arg.trigger_button == Qt::RightButton) {
                        interactionState.endGlobalDragPreview();
                    }
                },
                // 编辑相关
                // 复制
                [&](const CopyCommand& arg) {
                    setClipBoard(arg.entities, true);
                },
                // 剪切
                [&](const CutCommand& arg) {
                    setClipBoard(arg.entities, false);
                },
                // 粘贴
                [&](const PasteCommand& arg) { pasteEntities(); },
                // 粘贴
                [&](const MirrorCommand& arg) { mirrirEntities(); },
                // 选中相关
                [&](const StartSelectCommand& arg) {
                    auto timepos = glm::vec2{
                        arg.common_info.start_mouse_pos.x,
                        maintrack_converter->distanceToTime(
                            canvas_height - arg.common_info.start_mouse_pos.y -
                                judgeline_absolute_y,
                            presentation_canvas_time)};
                    interactionState.startNewSelectArea(
                        arg.append, arg.trigger_button,
                        arg.common_info.start_mouse_pos, timepos);
                },
                [&](const UpdateSelectAreaCommand& arg) {
                    interactionState.updateSelectArea(
                        arg.current_buttons, *maintrack_converter,
                        canvas_height, judgeline_absolute_y,
                        presentation_canvas_time);
                },
                [&](const EndSelectCommand& arg) {
                    interactionState.endNewSelectArea(arg.end_button);
                },
                // 清理拖动实体状态
                [&](const ClearDragStateCommand& arg) {
                    interactionState.endDrag();
                },
                // 放置物件
                // 放置单物件
                [&](const StartCreateNewNormalNoteCommand& arg) {
                    // 开始创建新单键物件
                    interactionState.startCreate(CreateMode::Normal);
                    // 立即更新一次创建节点
                    updateCreateNode();
                },
                // 放置复合物件
                [&](const StartCreateNewCompositeNoteCommand& arg) {
                    // 开始创建新复合键物件
                    interactionState.startCreate(CreateMode::Composite);
                    // 立即更新一次创建节点
                    updateCreateNode();
                },
                // 更新创建节点
                [&](const UpdateCreateNodeCommand& arg) { updateCreateNode(); },

                // 确认放置物件
                [&](const ConfirmCreateNewNoteCommand& arg) {
                    endCreateNewNote();
                    // StartCreateNewNormalNote();
                },
                // 所有开始拖拽命令
                // 匹配所有包含 hit_info 的拖拽命令
                [&](const StartDragCommand& arg) {
                    startDragEntities({arg.hit_info.source_entity},
                                      arg.hit_info);
                },

                // 拖拽选择集
                [&](const StartDragSelectionCommand& arg) {
                    startDragEntities(arg.selection, arg.hit_info,
                                      arg.move_only);
                },

                // 结束拖拽命令
                [&](const EndDragCommand& arg) { endDrag(); },

                // 删除相关
                // 标记删除命令
                [&](const MarkDeleteCommand& arg) {
                    markDeleteEntities(arg.selection, arg.hit_info);
                },

                // 确认删除命令
                [&](const ConfirmDeleteCommand& arg) {
                    confirmDeleteEntities(arg.confirm);
                },

            },
            command);
    }

   private:
    entt::registry& registry;
    ToolSystem& system;
    ToolInteractionState& interactionState;
    MMapEditor& mapEditor;
    MMap* map;

    const TimePixelConverter* maintrack_converter;
    const TimePixelConverter* preview_converter;
    MapCanvasInfo* info;
    glm::vec4 all_tracks_rect;
    int track_count;
    float judgeline_absolute_y;
    float judgeline_pos_in_previewarea;
    float single_track_width;
    float canvas_height;
    double presentation_canvas_time;

    // 转化像素位置到谱面坐标系
    MapAxis getPixelMapAxis(const glm::vec2& pixel) const {
        MapAxis axis;
        auto map = info->editorInfo.map;
        auto& beat_timeline = map->beat_timeline();
        auto& beat_info = map->beat_info();
        axis.time = maintrack_converter->distanceToTime(
            canvas_height - pixel.y - judgeline_absolute_y,
            presentation_canvas_time);
        axis.mousetime = axis.time;
        axis.y = maintrack_converter->timeToPixel(
            axis.time, presentation_canvas_time, info);
        // 查询最近的分拍
        auto divinfo =
            findNearestDivisorLine(axis.time, beat_timeline, beat_info);
        if (divinfo.is_valid()) {
            // 更新吸附到最近的分拍
            axis.time = divinfo.divisor_time;
            axis.y = maintrack_converter->timeToPixel(
                axis.time, presentation_canvas_time, info);
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

    void markDeleteEntities(const std::unordered_set<entt::entity>& selections,
                            MeshPartInfo part = {}) {
        // 更新 TIS 的删除标记集
        interactionState.startDeleteCheck(selections);
        // 为所有被拖拽的实体附加删除标记组件
        for (auto& entity : selections) {
            registry.emplace_or_replace<DeleteMarkComponent>(entity);
        }
    }

    void confirmDeleteEntities(bool confirm) {
        // 删除标记组件
        std::vector<entt::entity> entities;
        auto view = registry.view<DeleteMarkComponent>();
        for (const auto& e : view) {
            entities.push_back(e);
        }
        registry.clear<DeleteMarkComponent>();

        // 确定删除
        if (confirm) {
            // 收集uuid/删除实体
            std::unordered_set<NoteUUID> uuids_toremove;
            for (const auto& e : entities) {
                auto& [track, uuid] = registry.get<NoteComponent>(e);
                uuids_toremove.insert(uuid);
                if (registry.valid(e)) {
                    registry.destroy(e);
                }
                // 获取空间网格值
                const auto part = system.find_MeshPartInfo(e);
                // 删除空间索引
                system.get_mesh_info_tree().remove(part);
            }
            // 清理交互的hover状态
            interactionState.setHover(std::nullopt);
            // 删除物件
            mapEditor.deleteNotes(uuids_toremove);
        }

        interactionState.endDeleteCheck();
    }

    void updateCreateNode() {
        // interactionState.startCreate(CreateMode::Composite);
        auto mousePos = interactionState.getMouseState().current_pos;
        auto axis = getPixelMapAxis(mousePos);
        // 验证更新
        auto validity =
            axis.time >= 0 && axis.track >= 0 && axis.track < track_count;
        interactionState.setCreateValidity(validity);
        if (validity) {
            interactionState.updateCreateNode(axis);
        }
    }

    void endCreateNewNote() {
        auto createState = interactionState.getCreateState();
        // 结束创建新物件状态
        interactionState.endCreate();
        if (createState.is_valid) {
            // 创建新物件
            if (createState.mode == CreateMode::Normal) {
                // 创建单键
                auto axis = createState.createState_nodes.front();
                mapEditor.createNoteAt(axis.time, axis.track);
            } else if (createState.mode == CreateMode::Composite) {
                auto& axies = createState.createState_nodes;
                if (axies.size() == 2) {
                    // 只创建hold或slide
                    auto& node1 = axies.front();
                    auto& node2 = axies.back();
                    auto time_same = node1.time == node2.time;
                    auto track_same = node1.track == node2.track;
                    if (time_same) {
                        // 创建slide物件
                        mapEditor.createFlickAt(node1.time, node1.track,
                                                node2.track - node1.track);
                        return;
                    }
                    if (track_same) {
                        // 创建hold物件
                        mapEditor.createHoldAt(node1.time, node1.track,
                                               node2.time - node1.time);
                        return;
                    }
                } else {
                    // 创建复合键
                    mapEditor.createCompositeWithAxis(axies);
                }
            }
        }
    }

    void setClipBoard(std::unordered_set<entt::entity> entities, bool is_copy) {
        std::unordered_set<NoteUUID> uuids;
        for (const auto& e : entities) {
            if (registry.valid(e)) {
                auto& [track, uuid] = registry.get<NoteComponent>(e);
                uuids.insert(uuid);
            }
        }
        interactionState.setClipBoard(uuids, is_copy);
        qDebug() << (is_copy ? "已复制" : "已剪切");
    }

    void pasteEntities() {
        auto clipboard = interactionState.getClipBoard();
        NoteUUID referenceUUID{InvalidNoteUUID};
        auto min_time = INT32_MAX;
        for (const auto& uuid : clipboard.uuids) {
            auto note = info->editorInfo.map->note_set().get_note(
                info->editorInfo.map->note_uuids().get_handle(uuid));
            if (note->timestamp() < min_time) {
                min_time = note->timestamp();
                referenceUUID = uuid;
            }
        }
        // 如果找不到任何有效的 Note，则无法操作
        if (referenceUUID == InvalidNoteUUID) {
            return;
        }
        if (clipboard.is_copy) {
            // 是复制
            mapEditor.copyNotesTo(clipboard.uuids, referenceUUID,
                                  presentation_canvas_time);
        } else {
            // 是剪切

            // 计算时间偏移量：目标时间 - 参考Note的原始时间
            const int64_t time_offset = presentation_canvas_time - min_time;

            // 准备 moveNotes 函数需要的参数
            std::unordered_map<NoteUUID, std::pair<int64_t, int>> notes_to_move;
            notes_to_move.reserve(clipboard.uuids.size());

            for (const auto& uuid : clipboard.uuids) {
                auto handle =
                    info->editorInfo.map->note_uuids().get_handle(uuid);
                auto note = info->editorInfo.map->note_set().get_note(handle);
                if (!note) continue;

                // 计算每个 Note 的新时间戳
                auto new_timestamp = note->timestamp() + time_offset;
                // 轨道位置保持不变
                int track = note->trackpos();

                notes_to_move[uuid] = {new_timestamp, track};
            }

            // 调用 moveNotes
            mapEditor.moveNotes(notes_to_move);

            // 清空剪贴板，剪切是一次性操作
            interactionState.setClipBoard({}, true);
        }
        qDebug() << "已粘贴";
    }

    void mirrirEntities() {
        // 镜像物件
        auto selections = interactionState.getSelectionState();
        auto& left_selections =
            selections.all_selected_entities[Qt::LeftButton];
        if (!left_selections.empty()) {
            if (left_selections.size() == 1) {
                auto uuid =
                    registry.get<NoteComponent>(*left_selections.begin())
                        .sourceUUID;
                mapEditor.mirrorNote(uuid);
            } else {
                std::unordered_set<NoteUUID> uuids;
                for (const auto& e : left_selections) {
                    uuids.insert(registry.get<NoteComponent>(e).sourceUUID);
                }
                mapEditor.mirrorNotes(uuids);
            }
        }
    }

    void startDragEntities(const std::unordered_set<entt::entity>& selections,
                           MeshPartInfo part = {}, bool move_only = false) {
        std::unordered_map<entt::entity, MapAxis> selections_with_ress;
        std::unordered_map<entt::entity,
                           std::unordered_map<entt::entity, MapAxis>>
            subject_selections_with_ress;
        // 获取所有选中物件的原始位置
        for (const auto& selected_entity : selections) {
            auto& [track, uuid] = registry.get<NoteComponent>(selected_entity);
            auto& [time] = registry.get<TimeComponent>(selected_entity);
            MapAxis source_axis{time, time, track};
            source_axis.y = maintrack_converter->timeToPixel(
                source_axis.time, presentation_canvas_time, info);
            source_axis.x =
                all_tracks_rect.x +
                (float(source_axis.track) + 0.5f) * single_track_width;
            selections_with_ress.try_emplace(selected_entity, source_axis);
            // 若为复合物件则放子实体坐标到从属集合中
            if (registry.all_of<CompositeRootComponent>(selected_entity)) {
                auto& subject_selections_with_res =
                    subject_selections_with_ress[selected_entity];
                auto& [children, total_duration] =
                    registry.get<CompositeRootComponent>(selected_entity);
                for (const auto& child_e : children) {
                    auto& [child_track, child_uuid] =
                        registry.get<NoteComponent>(child_e);
                    auto& [child_time] = registry.get<TimeComponent>(child_e);
                    MapAxis child_source_axis{child_time, child_time,
                                              child_track};
                    child_source_axis.y = maintrack_converter->timeToPixel(
                        child_source_axis.time, presentation_canvas_time, info);
                    child_source_axis.x =
                        all_tracks_rect.x +
                        (float(child_source_axis.track) + 0.5f) *
                            single_track_width;
                    subject_selections_with_res.try_emplace(child_e,
                                                            child_source_axis);
                }
            }
        }

        // 更新 TIS 的选择集
        interactionState.setSelection(Qt::LeftButton, selections);

        if (move_only) {
            // 仅移动时移除部位信息防止触发编辑渲染
            part.part = NotePart::NONE;
        }
        // 更新 TIS 的拖拽状态
        interactionState.startDrag(DragMode::Entity, part,
                                   // 选中列表
                                   selections_with_ress,
                                   subject_selections_with_ress);

        // 为所有被拖拽的实体附加虚影组件
        for (auto& entity : selections) {
            registry.emplace_or_replace<GhostComponent>(entity);
        }
    }

    void endDrag() {
        // 完成拖拽编辑
        // 获取拖拽的最终状态和有效性
        DragState drag_info = interactionState.getDragState();

        bool was_drag_valid = drag_info.is_valid;

        if (was_drag_valid) {
            // 操作有效，在这里执行永久性的数据修改
            // 这部分逻辑可能很复杂，需要根据 drag_info.mode 和
            // drag_info.drag_start_hit.part 来决定如何修改
            // NoteCollection 例如:
            if (drag_info.mode == DragMode::Entity) {
                if (drag_info.dragged_entitiesWithRes.size() == 1) {
                    // 获取新的位置
                    auto entity =
                        drag_info.dragged_entitiesWithRes.begin()->first;
                    auto [track, uuid] = registry.get<NoteComponent>(entity);
                    auto [time] = registry.get<TimeComponent>(entity);
                    auto mapAxisRes =
                        drag_info.dragged_entitiesWithRes.begin()->second;
                    if (drag_info.drag_start_hit.part == NotePart::NONE ||
                        drag_info.drag_start_hit.part == NotePart::HEAD ||
                        drag_info.drag_start_hit.part == NotePart::HOLD_HEAD ||
                        drag_info.drag_start_hit.part == NotePart::SLIDE_HEAD) {
                        // 向编辑器应用修改
                        mapEditor.moveNote(uuid, mapAxisRes.time,
                                           mapAxisRes.track);

                    } else if (drag_info.drag_start_hit.part ==
                               NotePart::HOLD_END) {
                        // ... 计算并应用新的 duration ...
                        auto duration = mapAxisRes.time - time;
                        // 向编辑器应用修改
                        mapEditor.updateHold(uuid, duration);
                    } else if (drag_info.drag_start_hit.part ==
                               NotePart::SLIDE_END) {
                        // ... 计算并应用新的 delta track ...
                        auto delta_track = mapAxisRes.track - track;
                        // 向编辑器应用修改
                        mapEditor.updateSlide(uuid, delta_track);
                    }
                    // else if () {
                    //
                    // }
                    // 附加脏组件(下一帧更新)
                    registry.emplace<DirtyNoteMarkComponent>(entity);
                } else {
                    // 拖拽多个
                    std::unordered_map<NoteUUID, std::pair<int64_t, int>>
                        notes_to_move;
                    for (const auto& [e, axis] :
                         drag_info.dragged_entitiesWithRes) {
                        // 获取新的位置
                        auto [track, uuid] = registry.get<NoteComponent>(e);
                        auto [time] = registry.get<TimeComponent>(e);
                        notes_to_move.try_emplace(
                            uuid,
                            std::pair<int64_t, int>(axis.time, axis.track));
                        // 附加脏组件(下一帧更新)
                        registry.emplace<DirtyNoteMarkComponent>(e);
                    }
                    mapEditor.moveNotes(notes_to_move);
                }
            }
        } else {
            // 恢复位置-不做修改
        }

        // 无论如何，都结束拖拽状态
        // 清理所有被拖拽实体的虚影组件
        for (auto [entity, mapaxis] : drag_info.dragged_entitiesWithRes) {
            if (registry.valid(entity)) {
                registry.remove<GhostComponent>(entity);
            }
        }

        // 重置 TIS 中的拖拽状态
        interactionState.endDrag();
    }
};

#endif  // MMM_TOOLCOMMANDPROCESSOR_HPP
