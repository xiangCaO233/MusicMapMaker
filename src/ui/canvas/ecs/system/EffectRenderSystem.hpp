#ifndef MMM_EFFECTRENDERSYSTEM_HPP
#define MMM_EFFECTRENDERSYSTEM_HPP

#include <ecs/ECSCore.hpp>
#include <ecs/component/CoreComponents.hpp>
#include <ecs/component/EffectComponents.hpp>
#include <ecs/component/TransformComponents.hpp>
#include <ecs/system/TimePixelConverter.hpp>
#include <info/MapCanvasInfo.hpp>
#include <layer/ILayer.hpp>
#include <map/skin/MSkin.hpp>

class EffectRenderSystem {
   public:
    EffectRenderSystem() = default;
    void update(ECSCore& core, const MapCanvasInfo* info, ILayer* layer,
                RenderDataBuffer& buffer) const {
        const auto& registry = core.ecs_registry();
        const auto& skin = *info->editorInfo.skin;
        const float playspeed = info->baseInfo.scroll_speed;
        auto view = registry.view<EffectComponent, TrackIdentifierComponent>();
        for (auto entity : view) {
            const auto& [effect, track_id] =
                view.get<EffectComponent, TrackIdentifierComponent>(entity);

            // 如果特效是静默状态，则跳过
            if (effect.texture_type == EffectTextureType::NONE) {
                continue;
            }

            int frame_index = effect.frame_index;

            // 计算自上次重置以来经过的时间
            auto elapsed_ms =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - effect.last_reset_time)
                    .count();

            // 根据特效类型获取总时长和帧数
            float total_duration =
                skin.normal_hit_effect_duration * 1000 / playspeed;

            int frame_count;
            std::string dir;

            if (effect.texture_type == EffectTextureType::NORMAL) {
                frame_count = skin.nomal_hit_effect_frame_count;
                dir = skin.nomal_hit_effect_dir;
            } else if (effect.texture_type == EffectTextureType::SLIDE_END) {
                frame_count = skin.slide_hit_effect_frame_count;
                dir = skin.slide_hit_effect_dir;
            }

            // 如果播放时间已经超出，则不再渲染 (但实体不销毁)
            if (elapsed_ms >= total_duration) {
                // (可选)可以在这里把 effect.texture_type 设回
                // NONE，使其进入静默状态
                // registry.patch<EffectComponent>(entity, [](auto &eff){
                // eff.texture_type = EffectTextureType::NONE; });
                // 注意：patch是写操作，也应由主线程执行。所以这里最好是生成一个指令
                continue;
            }

            // 计算当前帧
            float progress = static_cast<float>(elapsed_ms) / total_duration;
            int current_frame = static_cast<int>(progress * (frame_count - 1));

            // ... (计算特效的X, Y坐标，生成渲染指令到 buffer) ...
        }
    }
};

#endif  // MMM_EFFECTRENDERSYSTEM_HPP
