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
                auto timing = std::get<Timing*>(e.editData);
                qDebug() << "接收到Timing更新编辑事件:目标timing[" << timing
                         << "]";
                if (timing->is_base_timing) {
                    std::lock_guard<std::mutex> lock(beatanalyze_mtx);
                    // 重新生成拍信息
                    layer_manager->map()->beat_timeline().clear();
                    layer_manager->map()->beat_info().clear();
                    layer_manager->map()->analyzeBeatInfo();
                }

                break;
            }
            default:
                break;
        }
    }
}
