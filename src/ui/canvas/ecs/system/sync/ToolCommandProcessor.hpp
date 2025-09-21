#ifndef MMM_TOOLCOMMANDPROCESSOR_HPP
#define MMM_TOOLCOMMANDPROCESSOR_HPP

#include <ecs/component/CoreComponents.hpp>
#include <ecs/system/TimePixelConverter.hpp>
#include <ecs/system/ToolSystem.hpp>
#include <info/NotePart.hpp>
#include <mmm/map/editor/MMapEditor.hpp>
#include <tool/ToolInteractionState.hpp>
#include <tool/command/ToolCommand.hpp>
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
                         ToolInteractionState& i, MMap* m,
                         const MapCanvasInfo* info,
                         TimePixelConverter* converter)
        : registry(r),
          system(s),
          interactionState(i),
          mapEditor(e),
          map(m),
          converter(converter),
          info(info) {
        all_tracks_rect = info->editorInfo.track_layout;
        track_count = info->editorInfo.map->base_metadata().track_count;
        single_track_width = all_tracks_rect.z / float(track_count);
        canvas_height = info->baseInfo.canvasSize.height();
        judgeline_absolute_y = canvas_height * info->baseInfo.judgeline_pos;
        presentation_canvas_time =
            info->realTimeInfo.current_time_info.presentation_canvas_time;
    }

    ~ToolCommandProcessor() = default;

    void process(const ToolCommand& command) {
        std::visit(
            overloaded{
                // 清理拖动实体状态
                [&](const ClearDragStateCommand& arg) {
                    interactionState.endDrag();
                },
                // 放置物件
                // 放置单物件
                [&](const StartCreateNewNormalNoteCommand& arg) {
                    startCreateNewNormalNote();
                },
                // 放置复合物件
                [&](const StartCreateNewCompositeNoteCommand& arg) {
                    startCreateNewCompositeNote();
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
                    startDragEntities(arg.selection);
                },

                // 结束拖拽命令
                [&](const EndDragCommand& arg) { endDrag(); },

                // 删除相关
                // 标记删除命令
                [&](const MarkDeleteCommand& arg) {
                    //
                    markDeleteEntities(arg.selection, arg.hit_info);
                },

                // 确认删除命令
                [&](const ConfirmDeleteCommand& arg) {
                    //
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

    TimePixelConverter* converter;
    const MapCanvasInfo* info;
    glm::vec4 all_tracks_rect;
    int track_count;
    float judgeline_absolute_y;
    float single_track_width;
    float canvas_height;
    double presentation_canvas_time;

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

    void startCreateNewNormalNote() {
        // 开始创建新单键物件
        interactionState.startCreate(CreateMode::Normal);
        // 立即更新一次创建节点
        updateCreateNode();
    }

    void startCreateNewCompositeNote() {
        // 开始创建新复合键物件
        interactionState.startCreate(CreateMode::Composite);
        // 立即更新一次创建节点
        updateCreateNode();
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

    void startDragEntities(const std::unordered_set<entt::entity>& selections,
                           MeshPartInfo part = {}) {
        // 更新 TIS 的选择集
        interactionState.setSelection(selections);

        // 更新 TIS 的拖拽状态
        interactionState.startDrag(
            DragMode::Entity,
            // 多选时没有单一的命中部位(不传入part参数/使用none)
            part,
            // 选中列表
            selections);
        // qDebug() << "startDrag:" << to_string(part.part);

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
                    if (drag_info.drag_start_hit.part == NotePart::HEAD ||
                        drag_info.drag_start_hit.part == NotePart::HOLD_HEAD ||
                        drag_info.drag_start_hit.part == NotePart::SLIDE_HEAD) {
                        // 应用新的位置
                        auto entity =
                            drag_info.dragged_entitiesWithRes.begin()->first;
                        auto [track, uuid] =
                            registry.get<NoteComponent>(entity);
                        auto mapAxisRes =
                            drag_info.dragged_entitiesWithRes.begin()->second;

                        // 向编辑器应用修改
                        mapEditor.moveNote(uuid, mapAxisRes.time,
                                           mapAxisRes.track);

                        // 附加脏组件
                        registry.emplace<DirtyMarkComponent>(entity);

                    } else if (drag_info.drag_start_hit.part ==
                               NotePart::HOLD_END) {
                        // ... 计算并应用新的 duration ...
                        // 应用新的位置
                        auto entity =
                            drag_info.dragged_entitiesWithRes.begin()->first;
                        auto [track, uuid] =
                            registry.get<NoteComponent>(entity);
                        auto [time] = registry.get<TimeComponent>(entity);
                        auto mapAxisRes =
                            drag_info.dragged_entitiesWithRes.begin()->second;
                        auto duration = mapAxisRes.time - time;
                        // 向编辑器应用修改
                        mapEditor.updateHold(uuid, duration);
                        // 附加脏组件
                        registry.emplace<DirtyMarkComponent>(entity);
                    } else if (drag_info.drag_start_hit.part ==
                               NotePart::SLIDE_END) {
                        // ... 计算并应用新的 delta track ...
                        // 应用新的位置
                        auto entity =
                            drag_info.dragged_entitiesWithRes.begin()->first;
                        auto [track, uuid] =
                            registry.get<NoteComponent>(entity);
                        auto mapAxisRes =
                            drag_info.dragged_entitiesWithRes.begin()->second;
                        auto delta_track = mapAxisRes.track - track;

                        // 向编辑器应用修改
                        mapEditor.updateSlide(uuid, delta_track);

                        // 附加脏组件
                        registry.emplace<DirtyMarkComponent>(entity);
                    }
                    // else if () {
                    //
                    // }
                } else {
                    // 拖拽多个
                }
            }
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
