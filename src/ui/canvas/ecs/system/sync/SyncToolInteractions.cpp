#include <ecs/system/sync/SyncSystem.hpp>
#include <ecs/system/sync/ToolCommandProcessor.hpp>
#include <info/NotePart.hpp>
#include <layer/MapLayerManager.hpp>
#include <mmm/map/MMap.hpp>
#include <tool/ThreadSafeQueue.hpp>
#include <tool/ToolInteractionState.hpp>

void updateHover(ToolSystem* toolSystem,
                 ToolInteractionState* toolInteractionState,
                 const MapCanvasInfo* info) {
    // 获取最新的鼠标位置状态 (由UI线程持续更新)
    MouseState mouse = toolInteractionState->getMouseState();
    glm::vec2 current_mouse_pos = mouse.current_pos;
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

        // 检查是否和旧的悬浮状态是同一个物体/部位
        if (!old_hover.has_value() ||
            old_hover.value().source_entity != hit_info.source_entity ||
            old_hover.value().child_entity != hit_info.child_entity ||
            old_hover.value().part != hit_info.part) {
            // 不一样，或者之前没有悬浮 -> 更新为新的悬浮状态
            toolInteractionState->setHover(hit_info);

            // toolSystem->get_mesh_info_tree().print_tree();

            qDebug() << "更新hover到uuid:" << hit_info.uuid;
            qDebug() << "检测到悬浮于最终来源实体:"
                     << static_cast<uint32_t>(hit_info.source_entity);
            qDebug() << "检测到悬浮于子实体:"
                     << static_cast<uint32_t>(hit_info.child_entity);

            qDebug() << "更新悬浮物件为"
                     << (note ? note->toString() : "创建中的物件");
            qDebug() << "悬浮的部位:" << to_string(hit_info.part);
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

// 处理所有工具指令
void processToolCommands(entt::registry& registry,
                         ThreadSafeQueue<ToolCommand>* toolCmdQ,
                         ToolSystem* system, MMapEditor* editor,
                         ToolInteractionState* toolInteractionState, MMap* map,
                         const MapCanvasInfo* info,
                         TimePixelConverter& converter) {
    auto cmds = toolCmdQ->drain();
    if (cmds.empty()) return;

    for (const auto& command : cmds) {
        ToolCommandProcessor processor(registry, *system, *editor,
                                       *toolInteractionState, map, info,
                                       &converter);
        processor.process(command);
    }
}

// 同步工具交互
void SyncSystem::updateToolInteractions(ECSCore& core,
                                        const MapCanvasInfo* info,
                                        MapLayerManager* layer_manager,
                                        TimePixelConverter& converter) const {
    // qDebug() << "同步系统->同步工具状态(at pretick)开始";
    auto toolSystem = layer_manager->get_tool_system();
    auto toolCmdQ = layer_manager->get_tool_cmdq();
    auto toolInteractionState = layer_manager->get_tool_interaction_state();

    // qDebug() << "同步系统->同步工具状态->处理工具指令(at pretick)开始";
    // 更新悬浮状态
    processToolCommands(core.ecs_registry(), toolCmdQ, toolSystem,
                        info->editorInfo.map->editor(), toolInteractionState,
                        info->editorInfo.map, info, converter);
    // qDebug() << "同步系统->同步工具状态->处理工具指令(at pretick)结束";

    // qDebug() << "同步系统->同步工具状态->处理实时悬浮检测(at pretick)开始";
    updateHover(toolSystem, toolInteractionState, info);
    // qDebug() << "同步系统->同步工具状态->处理实时悬浮检测(at pretick)结束";

    // qDebug() << "同步系统->同步工具状态(at pretick)结束";
}
