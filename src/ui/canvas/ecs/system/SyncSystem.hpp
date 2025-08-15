#ifndef MMM_SYNCSYSTEM_HPP
#define MMM_SYNCSYSTEM_HPP

#include <ecs/ECSCore.hpp>
#include <ecs/NoteComponents.hpp>
#include <ecs/RelationComponents.hpp>
#include <ecs/system/TimePixelConverter.hpp>
#include <entt.hpp>
#include <info/MapCanvasInfo.hpp>
#include <mmm/DataStructures.hpp>
#include <mmm/map/MMap.hpp>
#include <mmm/obj/Hold.hpp>
#include <mmm/obj/rm/Composite.hpp>
#include <mmm/obj/rm/Slide.hpp>

class SyncSystem {
   public:
    void update(ECSCore& core, const NoteCollection& notes,
                const MapCanvasInfo* info,
                const TimePixelConverter& converter) const {
        // 获取计算所需的上下文信息
        const auto& base_info = info->baseInfo;
        const auto& realtime_info = info->realTimeInfo;
        const int64_t current_time = realtime_info.current_canvas_time;
        const auto canvas_height = base_info.canvasSize.height();

        // 定义屏幕边界

        // 根据设定的坐标系 (Y=0在底部)，计算判定线的绝对像素位置。
        // 如果 judgeline_pos = 0.2f，意味着判定线在从下往上20%的高度。
        const auto judgeline_absolute_y =
            canvas_height * base_info.judgeline_pos;

        // 计算屏幕顶部和底部到判定线的“相对像素距离”。
        // 这些相对值将作为 converter 的输入。
        // 正值代表“未来”方向（在屏幕上是向上的）。
        // 负值代表“过去”方向（在屏幕上是向下的）。
        const auto pixel_y_top = canvas_height - judgeline_absolute_y;
        const auto pixel_y_bottom = 0.0f - judgeline_absolute_y;

        // 使用转换器计算时间边界
        const int64_t time_at_top =
            converter.pixelToTime(pixel_y_top, current_time);
        const int64_t time_at_bottom =
            converter.pixelToTime(pixel_y_bottom, current_time);

        // 应用预加载缓冲
        const int64_t query_start_time =
            time_at_bottom - base_info.view_timeMargin;
        const int64_t query_end_time = time_at_top + base_info.view_timeMargin;

        // 使用计算出的时间范围查询 NoteCollection
        const std::vector<NoteHandle> visible_handles =
            notes.query_range(query_start_time, query_end_time);

        // 执行ECS同步逻辑
        auto& registry = core.ecs_registry();
        auto& handle_map = core.handle_to_entity_map();

        std::unordered_set<NoteHandle, NoteHandle::Hash> current_visible_set(
            visible_handles.begin(), visible_handles.end());

        // 销毁不再可见的实体
        for (auto it = handle_map.begin(); it != handle_map.end();) {
            if (current_visible_set.contains(it->first)) {
                if (registry.valid(it->second)) {
                    registry.destroy(it->second);
                }
                it = handle_map.erase(it);
            } else {
                ++it;
            }
        }

        // 为新出现的可见Note创建实体
        for (const auto& handle : visible_handles) {
            if (!handle_map.contains(handle)) {
                const Note* note_data = notes.get_note(handle);
                if (!note_data) continue;
                // 从 note_data 填充组件
                handle_map[handle] = createEntity(registry, note_data, handle);
            }
        }
    }

    // 创建实体
    entt::entity createEntity(entt::registry& registry, const Note* note,
                              NoteHandle handle) const {
        auto note_entity = registry.create();
        // 附加note组件(time,track,source)
        registry.emplace<NoteComponent>(note_entity, note->timestamp(),
                                        note->trackpos(), handle);
        switch (note->notetype()) {
            case NoteType::HOLD: {
                auto hold_note = static_cast<const Hold*>(note);
                registry.emplace<HoldComponent>(note_entity,
                                                hold_note->duration());
                break;
            }
            case NoteType::SLIDE: {
                auto slide_note = static_cast<const Slide*>(note);
                registry.emplace<FlickComponent>(note_entity,
                                                 slide_note->delta_track());
            }
            case NoteType::COMPOSITE: {
                auto composed_note = static_cast<const Composite*>(note);
                std::vector<entt::entity> children;
                for (const auto& child_note : composed_note->children()) {
                    auto child_note_entity =
                        createEntity(registry, child_note.get(), {0, 0});
                    // 附加父实体组件
                    registry.emplace<ChildOfComponent>(child_note_entity,
                                                       note_entity);
                    // 添加实体到父实体的复合组件的子实体列表
                    children.push_back(child_note_entity);
                }
                // 父实体附加子实体列表组件
                registry.emplace<CompositeRootComponent>(note_entity, children);
            }
            default:
                break;
        }
        return note_entity;
    }
};

#endif  // MMM_SYNCSYSTEM_HPP
