#include <ecs/system/sync/SyncSystem.hpp>
#include <ecs/system/sync/ToolCommandProcessor.hpp>
#include <info/NotePart.hpp>
#include <layer/MapLayerManager.hpp>
#include <mmm/map/MMap.hpp>
#include <tool/ToolCommandQueue.hpp>
#include <tool/ToolInteractionState.hpp>

void updateHover(ToolSystem* toolSystem,
                 ToolInteractionState* toolInteractionState,
                 const MapCanvasInfo* info) {
    // 获取最新的鼠标位置状态 (由UI线程持续更新)
    MouseState mouse = toolInteractionState->getMouseState();
    glm::vec2 current_mouse_pos = mouse.current_pos;
    auto& notes = info->editorInfo.map->note_set();
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
        auto note = notes.get_note(hit_info.handle);

        // 检查是否和旧的悬浮状态是同一个物体/部位
        if (!old_hover.has_value() ||
            old_hover.value().source_entity != hit_info.source_entity ||
            old_hover.value().part != hit_info.part) {
            // 不一样，或者之前没有悬浮 -> 更新为新的悬浮状态
            toolInteractionState->setHover(hit_info);

            // qDebug() << "更新hover到id:" << hit_info.handle.index;
            // qDebug() << "检测到悬浮于实体:"
            //          << static_cast<uint32_t>(hit_info.source_entity);
            // qDebug() << "更新悬浮物件为" << note->toString();
            // qDebug() << "悬浮的部位:" << to_string(hit_info.part);
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

void processToolCommands(entt::registry& registry, ToolCommandQueue* toolCmdQ,
                         ToolSystem* system,
                         ToolInteractionState* toolInteractionState,
                         MMap* map) {
    auto cmds = toolCmdQ->drain();
    if (cmds.empty()) return;

    for (const auto& command : cmds) {
        ToolCommandProcessor processor(registry, *system, *toolInteractionState,
                                       map);
        processor.process(command);
    }
}

// 同步工具交互
void SyncSystem::updateToolInteractions(ECSCore& core,
                                        const MapCanvasInfo* info,
                                        MapLayerManager* layer_manager) const {
    // qDebug() << "同步系统->同步工具状态(at pretick)开始";
    auto toolSystem = layer_manager->get_tool_system();
    auto toolCmdQ = layer_manager->get_tool_cmdq();
    auto toolInteractionState = layer_manager->get_tool_interaction_state();

    // qDebug() << "同步系统->同步工具状态->处理工具指令(at pretick)开始";
    // 更新悬浮状态
    processToolCommands(core.ecs_registry(), toolCmdQ, toolSystem,
                        toolInteractionState, info->editorInfo.map);
    // qDebug() << "同步系统->同步工具状态->处理工具指令(at pretick)结束";

    // qDebug() << "同步系统->同步工具状态->处理实时悬浮检测(at pretick)开始";
    updateHover(toolSystem, toolInteractionState, info);
    // qDebug() << "同步系统->同步工具状态->处理实时悬浮检测(at pretick)结束";

    // qDebug() << "同步系统->同步工具状态(at pretick)结束";
}
