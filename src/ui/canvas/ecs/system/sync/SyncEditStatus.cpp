#include <qlogging.h>

#include <ecs/system/sync/SyncSystem.hpp>
#include <layer/MapLayerManager.hpp>

// 同步编辑状态(处理编辑器事件)
void SyncSystem::updateEditStatus(ECSCore& core,
                                  MapLayerManager* layer_manager) {
    auto editEvents = layer_manager->get_edit_eventq()->drain();
    auto& registry = core.ecs_registry();
    for (const auto& e : editEvents) {
        switch (e.type) {
            case MMapEditEventType::NoteUpdated: {
                auto updated_e_it = core.uuid_to_entity_map().find(
                    std::get<NoteUUID>(e.editData));
                if (updated_e_it != core.uuid_to_entity_map().end()) {
                    registry.emplace<DirtyMarkComponent>(updated_e_it->second);
                } else {
                    qDebug() << "未知更新:NoteUUID:"
                             << std::get<NoteUUID>(e.editData);
                }

                break;
            }
            case MMapEditEventType::TimingUpdated: {
                qDebug() << "接收到Timing更新编辑事件:目标timing["
                         << std::get<Timing*>(e.editData) << "]";
                // 重建所有转换器

                break;
            }
            default:
                break;
        }
    }
}
