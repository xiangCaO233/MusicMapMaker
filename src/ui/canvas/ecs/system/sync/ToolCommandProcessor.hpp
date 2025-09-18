#ifndef MMM_TOOLCOMMANDPROCESSOR_HPP
#define MMM_TOOLCOMMANDPROCESSOR_HPP

#include <ecs/component/CoreComponents.hpp>
#include <ecs/system/ToolSystem.hpp>
#include <mmm/map/editor/MMapEditor.hpp>
#include <tool/ToolInteractionState.hpp>
#include <tool/command/ToolCommand.hpp>
#include <vector>

#include "info/NotePart.hpp"

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
                         ToolInteractionState& i, MMap* m)
        : registry(r), system(s), interactionState(i), mapEditor(e), map(m) {}

    ~ToolCommandProcessor() = default;

    void process(const ToolCommand& command) {
        std::visit(overloaded{
                       // 所有开始拖拽命令
                       // 匹配所有包含 hit_info 的拖拽命令
                       [&](const StartDragCommand& arg) {
                           startDragEntities({arg.hit_info.source_entity},
                                             arg.hit_info);
                       },
                       // 拖拽单物件头
                       // [&](const StartDragNormalNoteCommand& arg) {
                       //     startDragEntities({arg.hit_info.source_entity},
                       //     arg.hit_info);
                       // },

                       // 拖拽选择集
                       [&](const StartDragSelectionCommand& arg) {
                           startDragEntities(arg.selection);
                       },

                       // 结束拖拽命令
                       [&](const EndDragCommand& arg) { endDrag(); },

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
                if (drag_info.dragged_entities.size() == 1) {
                    if (drag_info.drag_start_hit.part == NotePart::HEAD ||
                        drag_info.drag_start_hit.part == NotePart::HOLD_HEAD ||
                        drag_info.drag_start_hit.part == NotePart::SLIDE_HEAD) {
                        // 应用新的位置
                        auto entity = drag_info.dragged_entities.begin()->first;
                        auto [track, uuid] =
                            registry.get<NoteComponent>(entity);
                        auto mapAxisRes =
                            drag_info.dragged_entities.begin()->second;

                        // 向编辑器应用修改
                        mapEditor.moveNote(uuid, mapAxisRes.time,
                                           mapAxisRes.track);

                        // 附加脏组件
                        registry.emplace<DirtyMarkComponent>(entity);

                    } else if (drag_info.drag_start_hit.part ==
                               NotePart::HOLD_END) {
                        // ... 计算并应用新的 duration ...
                        // 应用新的位置
                        auto entity = drag_info.dragged_entities.begin()->first;
                        auto [track, uuid] =
                            registry.get<NoteComponent>(entity);
                        auto [time] = registry.get<TimeComponent>(entity);
                        auto mapAxisRes =
                            drag_info.dragged_entities.begin()->second;
                        auto duration = mapAxisRes.time - time;
                        // 向编辑器应用修改
                        mapEditor.updateHold(uuid, duration);
                        // 附加脏组件
                        registry.emplace<DirtyMarkComponent>(entity);
                    } else if (drag_info.drag_start_hit.part ==
                               NotePart::SLIDE_END) {
                        // ... 计算并应用新的 delta track ...
                        // 应用新的位置
                        auto entity = drag_info.dragged_entities.begin()->first;
                        auto [track, uuid] =
                            registry.get<NoteComponent>(entity);
                        auto mapAxisRes =
                            drag_info.dragged_entities.begin()->second;
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
        for (auto [entity, mapaxis] : drag_info.dragged_entities) {
            if (registry.valid(entity)) {
                registry.remove<GhostComponent>(entity);
            }
        }

        // 重置 TIS 中的拖拽状态
        interactionState.endDrag();
    }
};

#endif  // MMM_TOOLCOMMANDPROCESSOR_HPP
