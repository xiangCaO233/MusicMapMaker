#ifndef MMM_SYNCSYSTEM_HPP
#define MMM_SYNCSYSTEM_HPP

#include <ecs/ECSCore.hpp>
#include <ecs/component/CoreComponents.hpp>
#include <ecs/component/EffectComponents.hpp>
#include <ecs/component/NoteComponents.hpp>
#include <ecs/component/RelationComponents.hpp>
#include <ecs/component/StateComponents.hpp>
#include <ecs/component/TimeLineComponents.hpp>
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
    // 同步特效实体
    void update(ECSCore& core, const NoteCollection& notes,
                const MapCanvasInfo* info,
                const TimePixelConverter& converter) const {
        auto& registry = core.ecs_registry();

        // --- 1. 计算本帧流逝的逻辑时间 (delta_time) ---
        const double delta_time_ms = static_cast<double>(
            info->realTimeInfo.current_time_info.logic_canvas_time -
            info->realTimeInfo.last_time_info.logic_canvas_time);

        // 在帧开始时，重置所有音效状态
        auto reset_view = registry.view<SoundStateComponent>();
        for (auto entity : reset_view) {
            auto& sound_state = reset_view.get<SoundStateComponent>(entity);
            // 清空 map，为本帧的累加做准备
            sound_state.pending_sounds.clear();
        }

        // 更新推进所有活动特效的帧索引
        auto effect_view = registry.view<EffectComponent>();
        auto skin = info->editorInfo.skin;
        for (auto entity : effect_view) {
            auto& effect = effect_view.get<EffectComponent>(entity);

            // 如果特效不活跃，则跳过
            if (effect.duration <= 0.0) {
                effect.texture_type = EffectTextureType::NONE;  // 确保状态一致
                continue;
            }

            // a. 减少剩余持续时间
            effect.duration -= delta_time_ms;

            // b. 判断特效是否在本帧播放完毕
            if (effect.duration <= 0.0) {
                // 恢复为静默状态
                effect.texture_type = EffectTextureType::NONE;
                continue;  // 本帧不渲染
            }
            // 递增动画帧索引
            ++effect.frame_index;
        }

        // 检查并触发特效请求
        const auto& presentation_time =
            info->realTimeInfo.current_time_info.presentation_canvas_time;
        const auto& last_presentation_time =
            info->realTimeInfo.last_time_info.presentation_canvas_time;

        // 计算出用于特效判断的逻辑时间
        const double effect_time =
            presentation_time +
            (info->realTimeInfo.offset_info.effect_static_offset_ms +
             info->realTimeInfo.offset_info.effect_offset_ms) *
                info->realTimeInfo.audio_playback_rate;
        const double last_effect_time =
            last_presentation_time +
            (info->realTimeInfo.offset_info.effect_static_offset_ms +
             info->realTimeInfo.offset_info.effect_offset_ms) *
                info->realTimeInfo.audio_playback_rate;

        // 遍历所有可见的Note实体
        auto view = registry.view<NoteComponent, TimeComponent>();
        for (auto& entity : view) {
            const auto& [time] = registry.get<TimeComponent>(entity);

            // 使用偏移后的时间进行判断
            // Note的时间戳，是否在本帧“特效时间”前进的区间内
            if (time > last_effect_time && time <= effect_time) {
                // 这个Note需要触发特效
                const auto& [track, handle] =
                    registry.get<NoteComponent>(entity);
                auto note = notes.get_note(handle);
                if (!note) continue;

                // qDebug() << "time[" << time << "],track[" << track
                //          << "]需要触发特效";
                // 查找对应轨道的特效实体并更新它
                auto effect_view =
                    registry.view<EffectComponent, SoundStateComponent,
                                  TrackIdentifierComponent>();
                for (auto effect_entity : effect_view) {
                    const auto& [track] =
                        registry.get<TrackIdentifierComponent>(effect_entity);

                    if (track == note->trackpos()) {
                        // 找到了这条轨道的特效实体！

                        // 更新视觉状态：重置时间和纹理类型
                        auto& effect =
                            registry.get<EffectComponent>(effect_entity);

                        effect.duration =
                            skin->normal_hit_effect_duration * 1000.f;
                        effect.texture_type = EffectTextureType::NORMAL;
                        effect.frame_index = 0;

                        // 根据 Note 类型决定新的纹理特效目录和持续时间
                        if (note->notetype() == NoteType::SLIDE) {
                            effect.texture_type = EffectTextureType::SLIDE_END;
                            effect.duration = skin->normal_hit_effect_duration;
                        } else if (note->notetype() == NoteType::HOLD) {
                            auto hold = static_cast<const Hold*>(note);
                            effect.duration = hold->duration();
                        }

                        // 叠加音效
                        auto& sound_state =
                            registry.get<SoundStateComponent>(effect_entity);
                        // 如果已有同类请求，则在其上累加；如果没有，则创建新的
                        // 根据 Note 类型决定音效类型
                        SoundEffectType sound_to_play =
                            SoundEffectType::COMMON_HIT;
                        if (note->notetype() == NoteType::SLIDE) {
                            sound_to_play = SoundEffectType::SLIDE;
                        }
                        // 直接在 map 中增加对应音效的计数
                        sound_state.pending_sounds[sound_to_play]++;

                        // 找到并处理后，跳出查找循环
                        break;
                    }
                }
            }
        }
    }
    // 同步物件/拍/时间点实体
    void update(ECSCore& core, const NoteCollection& notes,
                const TimingMap& timings, const BeatTimeline& beatTimeLine,
                const BeatInfo& beatInfo, const MapCanvasInfo* info,
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
        // 同步Note实体
        sync_notes(core, notes, info, query_start_time, query_end_time);

        // 同步Timing 实体
        sync_timings(core, timings, query_start_time, query_end_time);

        // 同步拍实体
        sync_beats(core, beatTimeLine, beatInfo, info, query_start_time,
                   query_end_time);
    }

    void sync_notes(ECSCore& core, const NoteCollection& notes,
                    const MapCanvasInfo* info, const int64_t query_start_time,
                    const int64_t query_end_time) const {
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
            // 不处于当前可见实体集合中/且不处于选中集合中/且不为hoverd
            if (!current_visible_set.contains(it->first) &&
                !info->realTimeInfo.selected_mark_buffer.contains(it->second) &&
                it->second != info->realTimeInfo.hovered_info.e) {
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

        // 同步更新选中和悬浮组件
        // 清理上一帧的
        registry.clear<HoveredComponent>();
        registry.clear<SelectedComponent>();
        // 添加当前帧的
        if (info->realTimeInfo.hovered_info.has_hovered_entity) {
            registry.emplace<HoveredComponent>(
                info->realTimeInfo.hovered_info.e);
        }
        for (const auto& e : info->realTimeInfo.selected_mark_buffer) {
            registry.emplace<SelectedComponent>(e);
        }
    }

    void sync_timings(ECSCore& core, const TimingMap& timings,
                      const int64_t query_start_time,
                      const int64_t query_end_time) const {
        auto& registry = core.ecs_registry();
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

    void sync_beats(ECSCore& core, const BeatTimeline& beatTimeLine,
                    const BeatInfo& beatInfo, const MapCanvasInfo* info,
                    const int64_t query_start_time,
                    const int64_t query_end_time) const {
        auto& registry = core.ecs_registry();
        auto& beat_handle_map = core.handle_to_beatentity_map();
        // --- 1. 高效收集当前可见的 Beat 句柄 (时间戳) ---
        std::unordered_set<BeatHandle> visible_beat_handles;

        // 因为 beatTimeLine 是有序的，我们可以使用二分查找快速定位起点
        // std::lower_bound 找到第一个不小于 query_start_time 的元素
        auto start_it =
            std::lower_bound(beatTimeLine.begin(), beatTimeLine.end(),
                             static_cast<uint32_t>(query_start_time));
        // 如果找到的不是第一个元素，就将迭代器向前移动一个位置
        if (start_it != beatTimeLine.begin()) {
            --start_it;
        }
        // c. 遍历扩展后的范围，直到超出可见范围的末尾
        auto end_it = beatTimeLine.end();  // 预设为结尾
        bool extended_end = false;         // 标记是否已经向后扩展了一个

        for (auto it = start_it; it != beatTimeLine.end(); ++it) {
            const BeatHandle& timestamp = *it;

            // 将当前拍加入可见集合
            visible_beat_handles.insert(timestamp);

            // d. *** 核心修正：向后扩展一个拍 ***
            // 当我们第一次遍历到超出 query_end_time 的拍时...
            if (timestamp > query_end_time && !extended_end) {
                extended_end = true;     // 标记我们已经包含了这个“额外”的拍
                end_it = std::next(it);  // 记录下一次循环应该结束的位置
            }

            // 如果已经向后扩展过了，并且当前迭代器到达了记录的结束位置，就跳出循环
            if (extended_end && it == std::prev(end_it)) {
                break;
            }
        }

        // --- 2. 销毁不再可见的 Beat 实体 ---
        for (auto it = beat_handle_map.begin(); it != beat_handle_map.end();) {
            // 这里还需要考虑编辑器交互状态，我们暂时简化
            if (!visible_beat_handles.contains(it->first)) {
                if (registry.valid(it->second)) {
                    registry.destroy(it->second);
                }
                it = beat_handle_map.erase(it);
            } else {
                ++it;
            }
        }

        // --- 3. 创建新出现的 Beat 实体 ---
        for (const BeatHandle& handle : visible_beat_handles) {
            if (!beat_handle_map.contains(handle)) {
                // 从 BeatInfo 中查找详细信息
                auto beat_info_it = beatInfo.find(handle);
                if (beat_info_it != beatInfo.end()) {
                    const Beat* beat_data = &(beat_info_it->second);
                    beat_handle_map[handle] =
                        createBeatEntity(registry, beat_data);
                }
            }
        }
    }

    // 创建拍实体
    entt::entity createBeatEntity(entt::registry& registry,
                                  const Beat* beat) const {
        auto beat_entity = registry.create();

        // 附加 TimeComponent，以便 TimeSystem 处理它的Y坐标
        registry.emplace<TimeComponent>(beat_entity, beat->beat_start);

        // 附加 BeatComponent，存储分拍数等特有信息
        registry.emplace<BeatComponent>(beat_entity, beat->divisors,
                                        beat->beat_length);

        return beat_entity;
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
};

#endif  // MMM_SYNCSYSTEM_HPP
