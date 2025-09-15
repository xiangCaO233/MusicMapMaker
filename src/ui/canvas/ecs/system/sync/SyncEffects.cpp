#include <ecs/system/sync/SyncSystem.hpp>

// 同步特效实体
void SyncSystem::updateEffects(ECSCore& core, const NoteCollection& notes,
                               const NoteIDManager& uuidManager,
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
            const auto& [track, uuid] = registry.get<NoteComponent>(entity);
            auto note = notes.get_note(uuidManager.get_handle(uuid));
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
                    auto& effect = registry.get<EffectComponent>(effect_entity);

                    effect.duration = skin->normal_hit_effect_duration * 1000.f;
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
                    SoundEffectType sound_to_play = SoundEffectType::COMMON_HIT;
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
