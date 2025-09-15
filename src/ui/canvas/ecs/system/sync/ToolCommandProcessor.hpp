#ifndef MMM_TOOLCOMMANDPROCESSOR_HPP
#define MMM_TOOLCOMMANDPROCESSOR_HPP

#include <ecs/system/ToolSystem.hpp>
#include <tool/ToolInteractionState.hpp>
#include <tool/command/ToolCommand.hpp>

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
    ToolCommandProcessor(entt::registry& r, ToolSystem& s,
                         ToolInteractionState& i, MMap* m)
        : registry(r), system(s), interactionState(i), map(m) {}

    ~ToolCommandProcessor() = default;

    void process(const ToolCommand& command) {
        std ::visit(overloaded{
            // 所有开始拖拽命令
            // 匹配所有包含 hit_info 的拖拽命令
            [&](const auto& arg)
                requires requires { arg.hit_info; }
            { startDragEntities({arg.hit_info.source_entity}, arg.hit_info); },
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

            },
            command);
    }

   private:
    entt::registry& registry;
    ToolSystem& system;
    ToolInteractionState& interactionState;
    MMap* map;

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
                    if (drag_info.drag_start_hit.part == NotePart::HEAD) {
                        // ... 计算并应用新的位置 ...

                    } else if (drag_info.drag_start_hit.part ==
                               NotePart::HOLD_END) {
                        // ... 计算并应用新的 duration ...
                    }
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
