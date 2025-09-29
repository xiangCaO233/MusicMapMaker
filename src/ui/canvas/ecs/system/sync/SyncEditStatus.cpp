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
                    registry.emplace<DirtyNoteMarkComponent>(
                        updated_e_it->second);
                } else {
                    qDebug() << "未知更新:NoteUUID:"
                             << std::get<NoteUUID>(e.editData);
                }

                break;
            }
            case MMapEditEventType::NotesUpdated: {
                auto updated_note_uuids = std::get<NoteUUIDS>(e.editData);
                for (auto& uuid : updated_note_uuids) {
                    auto updated_e_it = core.uuid_to_entity_map().find(uuid);
                    if (updated_e_it != core.uuid_to_entity_map().end()) {
                        registry.emplace<DirtyNoteMarkComponent>(
                            updated_e_it->second);
                    } else {
                        qDebug() << "已不可见的更新:NoteUUID:" << uuid;
                    }
                }

                break;
            }
            case MMapEditEventType::TimingUpdated: {
                auto timing = std::get<Timing*>(e.editData);
                XINFO(std::format("接收到Timing更新编辑事件:目标timing时间[{}]",
                                  timing->timestamp));
                if (timing->is_base_timing) {
                    std::lock_guard<std::mutex> lock(beatanalyze_mtx);
                    // 重新生成拍信息
                    layer_manager->map()->beat_timeline().clear();
                    layer_manager->map()->beat_info().clear();
                    layer_manager->map()->analyzeBeatInfo();
                }

                break;
            }
            case MMapEditEventType::BeatUpdated: {
                auto beat = std::get<Beat*>(e.editData);
                XINFO(std::format("接收到Beat更新编辑事件:目标beattime[{}]",
                                  beat->beat_start));
                auto beatetit =
                    core.handle_to_beatentity_map().find(beat->beat_start);
                if (beatetit != core.handle_to_beatentity_map().end()) {
                    if (registry.valid(beatetit->second)) {
                        // 附加dirty组件
                        registry.emplace_or_replace<DirtyBeatMarkComponent>(
                            beatetit->second, beat);
                    } else {
                        XINFO(std::format(
                            "未知更新BeatEntity:[{}]",
                            static_cast<uint32_t>(beatetit->second)));
                    }
                }
                // if (timing->is_base_timing) {
                //     std::lock_guard<std::mutex> lock(beatanalyze_mtx);
                //     // 重新生成拍信息
                //     layer_manager->map()->beat_timeline().clear();
                //     layer_manager->map()->beat_info().clear();
                //     layer_manager->map()->analyzeBeatInfo();
                // }

                break;
            }
            default:
                break;
        }
    }
}
