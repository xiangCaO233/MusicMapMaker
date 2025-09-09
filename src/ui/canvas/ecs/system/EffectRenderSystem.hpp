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
#include <string>

class EffectRenderSystem {
   public:
    EffectRenderSystem() = default;
    void update(ECSCore& core, const MapCanvasInfo* info, ILayer* layer,
                RenderDataBuffer& buffer) const {
        const auto& registry = core.ecs_registry();
        const auto& skin = *info->editorInfo.skin;
        const auto& track_count =
            info->editorInfo.map->base_metadata().track_count;
        const auto& track_layout = info->editorInfo.track_layout;

        auto view = registry.view<EffectComponent, TrackIdentifierComponent>();
        for (auto entity : view) {
            const auto& [effect, track_id] =
                view.get<EffectComponent, TrackIdentifierComponent>(entity);

            auto texture_path = skin.nomal_hit_effect_dir;
            texture_path.append("/");
            texture_path.append(std::to_string(effect.frame_index));
            texture_path.append(".png");
            auto texture = layer->get(texture_path);

            // 如果特效是静默状态，则跳过
            if (effect.texture_type == EffectTextureType::NONE) {
                continue;
            }

            int frame_index = effect.frame_index;

            // ... (计算特效的X, Y坐标，生成渲染指令到 buffer) ...
        }
    }
};

#endif  // MMM_EFFECTRENDERSYSTEM_HPP
