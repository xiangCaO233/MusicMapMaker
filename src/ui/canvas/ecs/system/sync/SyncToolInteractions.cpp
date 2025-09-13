#include <ecs/system/sync/SyncSystem.hpp>
#include <layer/MapLayerManager.hpp>
#include <tool/ToolCommandQueue.hpp>
#include <tool/ToolInteractionState.hpp>

#include "info/NotePart.hpp"

// --- Helper for std::visit ---
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...)
    -> overloaded<Ts...>;  // C++17 class template argument deduction

void updateHover(ToolSystem* toolSystem,
                 ToolInteractionState* toolInteractionState) {
    // 获取最新的鼠标位置状态 (由UI线程持续更新)
    MouseState mouse = toolInteractionState->getMouseState();
    glm::vec2 current_mouse_pos = mouse.current_pos;
    // 使用 ToolSystem (四叉树) 在当前世界状态下重新查询
    // tool_system 的四叉树是在上一帧的 tick() 中更新的
    // 包含所有物体移动后的最新位置
    auto current_hit = toolSystem->query(current_mouse_pos);

    // 获取旧悬浮状态
    auto old_hover = toolInteractionState->getHover();

    // 进行比较和更新
    if (current_hit.has_value()) {
        // 当前鼠标下有物体
        const MeshPartInfo* hit_info = current_hit.value();

        // 检查是否和旧的悬浮状态是同一个物体/部位
        if (!old_hover.has_value() ||
            old_hover.value()->source_entity != hit_info->source_entity ||
            old_hover.value()->part != hit_info->part) {
            // 不一样，或者之前没有悬浮 -> 更新为新的悬浮状态
            toolInteractionState->setHover(hit_info);
            qDebug() << "更新hover到" << to_string(hit_info->part);
        }

        qDebug() << "当前hover:" << to_string(hit_info->part);
        // 如果一样，什么都不做，保持状态

    } else {
        // 当前鼠标下没有物体
        if (old_hover.has_value()) {
            // 但之前有悬浮 -> 清除悬浮状态
            toolInteractionState->setHover(std::nullopt);
        }
        // 如果之前也没有，就什么都不做
    }
}

// 无信息
const MeshPartInfo none{{}, {}, {}, NotePart::NONE, 0, {}};

void processToolCommands(entt::registry& registry, ToolCommandQueue* toolCmdQ,
                         ToolInteractionState* toolInteractionState) {
    auto cmds = toolCmdQ->drain();
    if (cmds.empty()) return;

    for (const auto& command : cmds) {
        std ::visit(overloaded{
            // --- 处理所有“开始拖拽”类的命令 ---
            // 这个 lambda 会匹配所有包含 hit_info 的拖拽命令
            [&](const auto& arg)
                requires requires { arg.hit_info; }
            {
                // 修改 ECS Registry: 附加虚影组件
                registry.emplace_or_replace<GhostComponent>(
                    arg.hit_info.source_entity);

                // 更新 TIS: 设置拖拽状态
                toolInteractionState->startDrag(DragMode::Entity, arg.hit_info,
                                                {arg.hit_info.source_entity});
            },

            // --- 单独处理“拖拽选择集”的命令 ---
            [&](const StartDragSelectionCommand& arg) {
                // 更新 TIS 的选择集
                toolInteractionState->setSelection(arg.selection);

                // 更新 TIS 的拖拽状态
                toolInteractionState->startDrag(
                    DragMode::Entity,
                    none,  // 多选时没有单一的命中部位
                    arg.selection);

                // 在 Registry 中为所有被拖拽的实体附加虚影组件
                for (auto entity : arg.selection) {
                    registry.emplace_or_replace<GhostComponent>(entity);
                }
            },

            // --- 处理“结束拖拽”的命令 ---
            [&](const EndDragCommand& arg) {
                // 获取拖拽的最终状态和有效性
                DragState drag_info = toolInteractionState->getDragState();

                bool was_drag_valid = drag_info.is_valid;

                if (was_drag_valid) {
                    // 操作有效，在这里执行永久性的数据修改
                    // 这部分逻辑可能很复杂，需要根据 drag_info.mode 和
                    // drag_info.drag_start_hit.part 来决定如何修改
                    // NoteCollection 例如:
                    if (drag_info.mode == DragMode::Entity) {
                        if (drag_info.drag_start_hit->part ==
                            NotePart::HOLD_END) {
                            // ... 计算并应用新的 duration ...
                        } else {
                            // ... 计算并应用新的位置 ...
                        }
                    }
                }

                // 无论如何，都结束拖拽状态
                // 清理所有被拖拽实体的虚影组件
                for (auto entity : drag_info.dragged_entities) {
                    if (registry.valid(entity)) {
                        registry.remove<GhostComponent>(entity);
                    }
                }
                // 重置 TIS 中的拖拽状态
                toolInteractionState->endDrag();
            },

            // --- 默认处理器 (可选) ---
            // 如果有命令没有被上面的 lambda 匹配，可以在这里处理
            // [&](const auto& arg) {
            //     // ... 默认逻辑 ...
            // }

            },
            command);
    }
}

// 同步工具交互
void SyncSystem::updateToolInteractions(ECSCore& core,
                                        const MapCanvasInfo* info,
                                        MapLayerManager* layer_manager) const {
    auto toolSystem = layer_manager->get_tool_system();
    auto toolCmdQ = layer_manager->get_tool_cmdq();
    auto toolInteractionState = layer_manager->get_tool_interaction_state();

    // 更新悬浮状态
    processToolCommands(core.ecs_registry(), toolCmdQ, toolInteractionState);
    updateHover(toolSystem, toolInteractionState);
}
