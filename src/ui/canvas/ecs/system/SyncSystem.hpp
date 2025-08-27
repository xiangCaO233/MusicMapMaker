#ifndef MMM_SYNCSYSTEM_HPP
#define MMM_SYNCSYSTEM_HPP

#include <ecs/ECSCore.hpp>
#include <ecs/component/CoreComponents.hpp>
#include <ecs/component/NoteComponents.hpp>
#include <ecs/component/RelationComponents.hpp>
#include <ecs/component/TimingComponents.hpp>
#include <ecs/component/TransformComponents.hpp>
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
                const TimingMap& timings, const MapCanvasInfo* info,
                const TimePixelConverter& converter) const {
        if (!info->editorInfo.map) return;

        // 获取计算所需的上下文信息
        const auto& base_info = info->baseInfo;
        const auto& realtime_info = info->realTimeInfo;
        const auto& current_time = realtime_info.presentation_canvas_time;
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
        const auto time_at_top =
            converter.pixelToTime(pixel_y_top, current_time);
        const auto time_at_bottom =
            converter.pixelToTime(pixel_y_bottom, current_time);

        // 应用预加载缓冲
        const auto query_start_time =
            time_at_bottom - base_info.view_timeMargin;
        const auto query_end_time = time_at_top + base_info.view_timeMargin;

        // qDebug() << "当前查询开始:" << query_start_time;
        // qDebug() << "当前查询结束:" << query_end_time;

        // 执行ECS同步逻辑
        auto& registry = core.ecs_registry();

        auto& handle_map = core.handle_to_entity_map();

        // ----------debug------------
        // auto current_entities = handle_map.size();
        // qDebug() << "初始实体句柄表大小:" << current_entities;
        // ----------debug------------

        // 使用计算出的时间范围查询 NoteCollection
        const std::vector<NoteHandle> visible_handles =
            notes.query_range(query_start_time, query_end_time);

        std::unordered_set<NoteHandle, NoteHandle::Hash> current_visible_set(
            visible_handles.begin(), visible_handles.end());

        // ----------debug------------
        // qDebug() << "当前可见实体数量:" << visible_handles.size();
        // ----------debug------------

        // 销毁不再可见的物件实体
        for (auto it = handle_map.begin(); it != handle_map.end();) {
            if (!current_visible_set.contains(it->first)) {
                if (registry.valid(it->second)) {
                    registry.destroy(it->second);
                }
                it = handle_map.erase(it);
            } else {
                ++it;
            }
        }
        // ----------debug------------
        // auto after_entities = handle_map.size();
        // qDebug() << "销毁不可见实体数量:" << after_entities -
        // current_entities;
        // ----------debug------------

        // 为新出现的可见Note创建实体
        for (const auto& handle : visible_handles) {
            if (!handle_map.contains(handle)) {
                const Note* note_data = notes.get_note(handle);
                if (!note_data) continue;
                // 从 note_data 填充组件
                handle_map[handle] =
                    createNoteEntity(registry, note_data, handle);
            }
        }

        // ----------debug------------
        // auto new_entities = handle_map.size();
        // qDebug() << "新建可见实体数量:" << new_entities - after_entities;
        // ----------debug------------

        // Timing 实体同步逻辑
        auto& timing_handle_map = core.handle_to_timingentity_map();

        // 收集当前可见的 TimingHandle
        std::unordered_set<TimingHandle, TimingHandle::Hash>
            visible_timing_handles;
        const auto& all_timing_points = timings.get_all_timing_points();

        // 遍历 std::map 来获取所有Timing点
        auto start_it = all_timing_points.upper_bound(query_start_time);
        if (start_it != all_timing_points.begin()) --start_it;

        for (auto it = start_it; it != all_timing_points.end(); ++it) {
            const auto& [timestamp, timings] = *it;
            // 超出范围立马停止
            if (timestamp > query_end_time) break;
            if (timestamp >= query_start_time) {
                for (const auto& timing : timings) {
                    // *** 为每个Timing点创建一个唯一的句柄 ***
                    visible_timing_handles.insert(
                        {timestamp, timing.beat_length});
                }
            }
        }

        // 销毁不再可见的 Timing 实体
        for (auto it = timing_handle_map.begin();
             it != timing_handle_map.end();) {
            if (!visible_timing_handles.contains(it->first)) {
                if (registry.valid(it->second)) registry.destroy(it->second);
                it = timing_handle_map.erase(it);
            } else {
                ++it;
            }
        }

        // 创建新出现的 Timing 实体
        for (const TimingHandle& handle : visible_timing_handles) {
            if (!timing_handle_map.contains(handle)) {
                // 因为句柄本身不足以定位Timing对象，我们还是需要查询map
                // 但由于Timing点很少，这次查询的开销可以忽略不计
                const auto& candidates =
                    all_timing_points.find(handle.timestamp);
                if (candidates != all_timing_points.end()) {
                    // 找到对应时间戳的timing点
                    const auto& timings = candidates->second;
                    for (const auto& timing_data : timings) {
                        // 确保 beat_length 也匹配
                        if (std::abs(timing_data.beat_length -
                                     handle.beat_length) < 1e-9) {
                            timing_handle_map[handle] =
                                createTimingEntity(registry, &timing_data);
                        }
                    }
                }
            }
        }
    }

    // 创建timing实体
    entt::entity createTimingEntity(entt::registry& registry,
                                    const Timing* timing) const {
        auto timing_entity = registry.create();
        // 附加Time组件
        registry.emplace<TimeComponent>(timing_entity, timing->timestamp);
        // 附加Timing组件
        registry.emplace<TimingComponent>(timing_entity, timing->bpm,
                                          timing->beat_length,
                                          timing->is_base_timing);
        return timing_entity;
    }

    // 创建物件实体
    entt::entity createNoteEntity(entt::registry& registry, const Note* note,
                                  NoteHandle handle) const {
        auto note_entity = registry.create();
        // 附加Time组件
        registry.emplace<TimeComponent>(note_entity, note->timestamp());

        // 附加note组件(time,track,source)
        registry.emplace<NoteComponent>(note_entity, note->trackpos(), handle);
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
                        createNoteEntity(registry, child_note.get(), {0, 0});
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
