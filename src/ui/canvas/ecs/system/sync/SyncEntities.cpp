#include <ecs/component/RelationComponents.hpp>
#include <ecs/system/sync/SyncSystem.hpp>
#include <layer/MapLayerManager.hpp>

// 创建拍实体
entt::entity createBeatEntity(entt::registry& registry, const Beat* beat) {
    auto beat_entity = registry.create();

    // 附加 TimeComponent，以便 TimeSystem 处理它的Y坐标
    registry.emplace<TimeComponent>(beat_entity, beat->beat_start);

    // 附加 BeatComponent，存储分拍数等特有信息
    registry.emplace<BeatComponent>(beat_entity, beat->divisors,
                                    beat->beat_length, beat->beat_index);

    return beat_entity;
}

// 更新物件实体
entt::entity updateNoteEntity(entt::registry& registry,
                              const entt::entity& note_entity,
                              const Note* note) {
    // 附加Time组件
    auto& [time] = registry.get<TimeComponent>(note_entity);
    time = note->timestamp();

    // 附加note组件(time,track,source)
    auto& [track, uuid] = registry.get<NoteComponent>(note_entity);
    track = note->trackpos();

    switch (note->notetype()) {
        case NoteType::HOLD: {
            auto hold_note = static_cast<const Hold*>(note);
            auto& [duration] = registry.get<HoldComponent>(note_entity);
            duration = hold_note->duration();
            break;
        }
        case NoteType::SLIDE: {
            auto slide_note = static_cast<const Slide*>(note);
            auto& [delta_track] = registry.get<FlickComponent>(note_entity);
            delta_track = slide_note->delta_track();
        }
        case NoteType::COMPOSITE: {
            auto composed_note = static_cast<const Composite*>(note);
            auto& [children] =
                registry.get<CompositeRootComponent>(note_entity);
            for (int i{0}; i < children.size(); ++i) {
                updateNoteEntity(registry, children[i],
                                 composed_note->children()[i].get());
            }
        }
        default:
            break;
    }
    return note_entity;
}

// 创建物件实体
entt::entity createNoteEntity(entt::registry& registry, const Note* note,
                              NoteUUID uuid) {
    auto note_entity = registry.create();
    // 附加Time组件
    registry.emplace<TimeComponent>(note_entity, note->timestamp());

    // 附加note组件(time,track,source)
    registry.emplace<NoteComponent>(note_entity, note->trackpos(), uuid);
    switch (note->notetype()) {
        case NoteType::HOLD: {
            auto hold_note = static_cast<const Hold*>(note);
            registry.emplace<HoldComponent>(note_entity, hold_note->duration());
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
                auto child_note_entity = createNoteEntity(
                    registry, child_note.get(), InvalidStableNoteID);
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

// 创建timing实体
entt::entity createTimingEntity(entt::registry& registry,
                                const Timing* timing) {
    auto timing_entity = registry.create();
    // 附加Time组件
    registry.emplace<TimeComponent>(timing_entity, timing->timestamp);
    // 附加Timing组件
    registry.emplace<TimingComponent>(timing_entity, timing->bpm,
                                      timing->beat_length,
                                      timing->is_base_timing);
    return timing_entity;
}
void sync_timings(ECSCore& core, const TimingMap& timings,
                  const int64_t query_start_time,
                  const int64_t query_end_time) {
    auto& registry = core.ecs_registry();
    // Timing 实体同步逻辑
    auto& timing_handle_map = core.handle_to_timingentity_map();

    // 收集当前可见的 TimingHandle
    const auto& all_timing_points = timings.get_all_timing_points();

    // 遍历 std::map 来获取所有Timing点
    // --- 1. 第一阶段：无条件收集所有可见的 TimingHandle ---
    std::vector<TimingHandle> all_visible_handles;

    auto start_it = all_timing_points.upper_bound(query_start_time);
    if (start_it != all_timing_points.begin()) --start_it;

    for (auto it = start_it; it != all_timing_points.end(); ++it) {
        const auto& [timestamp, timings_at_ts] = *it;
        if (timestamp > query_end_time) break;

        if (timestamp >= query_start_time) {
            for (const auto& timing : timings_at_ts) {
                all_visible_handles.push_back({timestamp, timing.beat_length});
            }
        }
    }
    // --- 2. 第二阶段：根据数量决定是否应用密度限制 ---
    std::unordered_set<TimingHandle, TimingHandle::Hash> final_visible_handles;
    const size_t DENSITY_LIMIT_THRESHOLD = 200;      // 定义阈值
    const size_t DENSITY_LIMIT_TIME_THRESHOLD = 20;  // 定义密度限制值

    if (all_visible_handles.size() > DENSITY_LIMIT_THRESHOLD) {
        // 数量超过阈值，需要进行密度限制
        qDebug() << "同屏timing数量过多[共" << all_visible_handles.size() << ">"
                 << DENSITY_LIMIT_THRESHOLD << "个],执行密度限制[间隔"
                 << DENSITY_LIMIT_TIME_THRESHOLD << "ms]";

        // 为了正确地进行密度限制，我们需要先对句柄排序
        // (因为同一个时间戳可能有多个timing，而 map 只保证了时间戳的顺序)
        std::sort(all_visible_handles.begin(), all_visible_handles.end(),
                  [](const TimingHandle& a, const TimingHandle& b) {
                      return a.timestamp < b.timestamp;
                  });

        final_visible_handles.reserve(DENSITY_LIMIT_THRESHOLD);  // 预留空间
        int32_t prev_time = -1000;  // 使用一个足够小的值初始化

        for (const auto& handle : all_visible_handles) {
            if (std::abs(handle.timestamp - prev_time) >=
                DENSITY_LIMIT_TIME_THRESHOLD) {
                final_visible_handles.insert(handle);
                prev_time = handle.timestamp;
            }
        }

    } else {
        //  数量在可接受范围内，全部显示
        final_visible_handles.insert(all_visible_handles.begin(),
                                     all_visible_handles.end());
    }

    // 销毁不再可见的 Timing 实体
    for (auto it = timing_handle_map.begin(); it != timing_handle_map.end();) {
        if (!final_visible_handles.contains(it->first)) {
            if (registry.valid(it->second)) registry.destroy(it->second);
            it = timing_handle_map.erase(it);
        } else {
            ++it;
        }
    }

    // 创建新出现的 Timing 实体
    for (const TimingHandle& handle : final_visible_handles) {
        if (!timing_handle_map.contains(handle)) {
            // 因为句柄本身不足以定位Timing对象，我们还是需要查询map
            // 但由于Timing点很少，这次查询的开销可以忽略不计
            const auto& candidates = all_timing_points.find(handle.timestamp);
            if (candidates != all_timing_points.end()) {
                // 找到对应时间戳的timing点
                const auto& timings = candidates->second;
                for (const auto& timing_data : timings) {
                    // 确保 beat_length 也匹配
                    if (std::abs(timing_data.beat_length - handle.beat_length) <
                        1e-9) {
                        timing_handle_map[handle] =
                            createTimingEntity(registry, &timing_data);
                    }
                }
            }
        }
    }
}

void sync_beats(ECSCore& core, const BeatTimeline& beatTimeLine,
                const BeatInfo& beatInfo, const MapCanvasInfo* info,
                const int64_t query_start_time, const int64_t query_end_time) {
    auto& registry = core.ecs_registry();
    auto& beat_handle_map = core.handle_to_beatentity_map();

    // 收集当前可见的 Beat 句柄 (时间戳)
    std::unordered_set<BeatHandle> visible_beat_handles;

    // 因为 beatTimeLine 是有序的，我们可以使用二分查找快速定位起点
    // std::lower_bound 找到第一个不小于 query_start_time 的元素
    auto start_it = std::lower_bound(
        beatTimeLine.begin(), beatTimeLine.end(),
        static_cast<uint32_t>(query_start_time < 0 ? 0 : query_start_time));
    // 如果找到的不是第一个元素，就将迭代器向前移动一个位置
    if (start_it != beatTimeLine.begin()) {
        --start_it;
    }
    // 遍历扩展后的范围，直到超出可见范围的末尾
    // 预设为结尾
    auto end_it = beatTimeLine.end();
    // 标记是否已经向后扩展了一个
    bool extended_end = false;

    BeatHandle prev_beat_handle{-100};
    for (auto it = start_it; it != beatTimeLine.end(); ++it) {
        const BeatHandle& timestamp = *it;
        if (std::abs(timestamp - prev_beat_handle) < 6) {
            prev_beat_handle = timestamp;
            continue;
        }

        // 将当前拍加入可见集合
        visible_beat_handles.insert(timestamp);

        // 向后扩展一个拍
        // 当第一次遍历到超出 query_end_time 的拍时
        if (timestamp > query_end_time && !extended_end) {
            // 标记已经包含了这个“额外”的拍
            extended_end = true;
            // 记录下一次循环应该结束的位置
            end_it = std::next(it);
        }
        prev_beat_handle = timestamp;

        // 如果已经向后扩展过了，并且当前迭代器到达了记录的结束位置，就跳出循环
        if (extended_end && it == std::prev(end_it)) {
            break;
        }
    }

    // 销毁不再可见的 Beat 实体
    for (auto it = beat_handle_map.begin(); it != beat_handle_map.end();) {
        // 这里还需要考虑编辑器交互状态
        if (!visible_beat_handles.contains(it->first)) {
            if (registry.valid(it->second)) {
                registry.destroy(it->second);
            }
            it = beat_handle_map.erase(it);
        } else {
            ++it;
        }
    }

    // 创建新出现的 Beat 实体
    for (const BeatHandle& handle : visible_beat_handles) {
        if (!beat_handle_map.contains(handle)) {
            // 从 BeatInfo 中查找详细信息
            auto beat_info_it = beatInfo.find(handle);
            if (beat_info_it != beatInfo.end()) {
                const Beat* beat_data = &(beat_info_it->second);
                beat_handle_map[handle] = createBeatEntity(registry, beat_data);
            }
        }
    }
}

void sync_notes(ECSCore& core, const NoteCollection& notes,
                const NoteIDManager& uuidManager,
                MapLayerManager* layer_manager, const MapCanvasInfo* info,
                const int64_t query_start_time, const int64_t query_end_time) {
    auto& registry = core.ecs_registry();
    auto& uuid_map = core.uuid_to_entity_map();

    // ----------debug------------
    // auto current_entities = handle_map.size();
    // qDebug() << "初始实体句柄表大小:" << current_entities;
    // ----------debug------------

    // 使用计算出的时间范围查询 NoteCollection
    const std::vector<NoteHandle> visible_handles =
        notes.query_range(query_start_time, query_end_time);

    // 获取到uuid集合
    std::unordered_set<NoteUUID> current_visible_uuidset;
    for (const auto& handle : visible_handles) {
        current_visible_uuidset.insert(uuidManager.get_id(handle));
    }

    // ----------debug------------
    // qDebug() << "当前可见实体数量:" << visible_handles.size();
    // ----------debug------------

    auto drag_info =
        layer_manager->get_tool_interaction_state()->getDragState();
    // 销毁不再可见的物件实体
    for (auto it = uuid_map.begin(); it != uuid_map.end();) {
        // 不处于当前可见实体集合中/且不处于拖动集合中
        if (!current_visible_uuidset.contains(it->first) &&
            !drag_info.dragged_entities.contains(it->second)) {
            if (registry.valid(it->second)) {
                registry.destroy(it->second);
            }
            it = uuid_map.erase(it);
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
    for (const auto& uuid : current_visible_uuidset) {
        if (!uuid_map.contains(uuid)) {
            auto handle = uuidManager.get_handle(uuid);
            const Note* note_data = notes.get_note(handle);
            if (!note_data) continue;
            // 从 note_data 填充组件
            uuid_map[uuid] = createNoteEntity(registry, note_data, uuid);
        }
    }

    // 更新脏物件
    auto dirty_view = registry.view<NoteComponent, DirtyMarkComponent>();
    for (const auto& e : dirty_view) {
        auto& [track, uuid] = registry.get<NoteComponent>(e);
        auto handle = uuidManager.get_handle(uuid);
        auto note_data = notes.get_note(handle);
        updateNoteEntity(registry, e, note_data);
    }
    registry.clear<DirtyMarkComponent>();

    // ----------debug------------
    // auto new_entities = handle_map.size();
    // qDebug() << "新建可见实体数量:" << new_entities - after_entities;
    // ----------debug------------
}
// 同步物件/拍/时间点实体
void SyncSystem::updateEntities(ECSCore& core, const NoteCollection& notes,
                                const NoteIDManager& uuidManager,
                                MapLayerManager* layer_manager,
                                const TimingMap& timings,
                                const BeatTimeline& beatTimeLine,
                                const BeatInfo& beatInfo,
                                const MapCanvasInfo* info,
                                const TimePixelConverter& converter) const {
    if (!info->editorInfo.map) return;

    // 获取计算所需的上下文信息
    const auto& base_info = info->baseInfo;
    const auto& realtime_info = info->realTimeInfo;
    const auto& current_time =
        realtime_info.current_time_info.presentation_canvas_time;
    const auto canvas_height = base_info.canvasSize.height();

    // 定义屏幕边界

    // 根据设定的坐标系 (Y=0在底部)，计算判定线的绝对像素位置。
    // 如果 judgeline_pos = 0.2f，意味着判定线在从下往上20%的高度。
    const auto judgeline_absolute_y = canvas_height * base_info.judgeline_pos;

    // 计算屏幕顶部和底部到判定线的“相对像素距离”。
    // 这些相对值将作为 converter 的输入。
    // 正值代表“未来”方向（在屏幕上是向上的）。
    // 负值代表“过去”方向（在屏幕上是向下的）。
    const auto pixel_y_top = canvas_height - judgeline_absolute_y;
    const auto pixel_y_bottom = 0.0f - judgeline_absolute_y;

    // 使用转换器计算时间边界
    const auto time_at_top = converter.pixelToTime(pixel_y_top, current_time);
    const auto time_at_bottom =
        converter.pixelToTime(pixel_y_bottom, current_time);

    // qDebug() << "Render target time range:[" << time_at_bottom << "~"
    //          << time_at_top << "]";

    // 应用预加载缓冲
    const auto query_start_time = time_at_bottom - base_info.view_timeMargin;
    const auto query_end_time = time_at_top + base_info.view_timeMargin;
    // qDebug() << "Render absolute(append margin) time range:["
    //          << query_start_time << "~" << query_end_time << "]";

    // qDebug() << "当前查询开始:" << query_start_time;
    // qDebug() << "当前查询结束:" << query_end_time;

    // 执行ECS同步逻辑
    // 同步Note实体
    sync_notes(core, notes, uuidManager, layer_manager, info, query_start_time,
               query_end_time);

    // 同步Timing 实体
    sync_timings(core, timings, query_start_time, query_end_time);

    // 同步拍实体
    sync_beats(core, beatTimeLine, beatInfo, info, query_start_time,
               query_end_time);

    // 排序beat实体
    auto& beat_group_to_sort = core.get_beat_group();
    if (beat_group_to_sort.size() < 101) {
        // 对 beat group 进行排序
        beat_group_to_sort.sort<TimeComponent>(
            [](const auto& lhs, const auto& rhs) {
                return lhs.timestamp < rhs.timestamp;
            });
    } else {
        // 放弃排序
        qDebug() << "可见拍过多[],放弃排序(可能造成拍层级混乱)";
    }
}
