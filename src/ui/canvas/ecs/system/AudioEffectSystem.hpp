#ifndef MMM_AUDIOEFFECTSYSTEM_HPP
#define MMM_AUDIOEFFECTSYSTEM_HPP
#include <ecs/ECSCore.hpp>
#include <ecs/component/CoreComponents.hpp>
#include <ecs/component/EffectComponents.hpp>
#include <ecs/component/TransformComponents.hpp>
#include <ecs/system/TimePixelConverter.hpp>
#include <info/MapCanvasInfo.hpp>
#include <layer/ILayer.hpp>
#include <map/skin/MSkin.hpp>

class AudioEffectSystem {
   public:
    AudioEffectSystem() = default;
    void update(ECSCore& core, const MapCanvasInfo* info) const {
        const auto& registry = core.ecs_registry();
        auto* audio_callback = info->audio_callback;
        auto& skin = *info->editorInfo.skin;

        // 遍历所有轨道的音效状态组件
        auto view = registry.view<SoundStateComponent>();
        for (auto entity : view) {
            auto& sound_state = view.get<SoundStateComponent>(entity);

            // 遍历 map 中所有待播放的音效
            for (auto& [sound_type, count] : sound_state.pending_sounds) {
                if (count > 0) {
                    // 播放 sound_strength (即 count) 次
                    for (uint32_t i = 0; i < count; ++i) {
                        audio_callback->play_oneshot(
                            skin.get_sound_effect(sound_type),
                            info->realTimeInfo.effect_volume);
                    }
                }
            }
        }
    }
};

#endif  // MMM_AUDIOEFFECTSYSTEM_HPP
