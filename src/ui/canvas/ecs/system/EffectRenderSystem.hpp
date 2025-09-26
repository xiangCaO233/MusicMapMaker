#ifndef MMM_EFFECTRENDERSYSTEM_HPP
#define MMM_EFFECTRENDERSYSTEM_HPP

#include <ecs/ECSCore.hpp>
#include <ecs/component/CoreComponents.hpp>
#include <ecs/component/EffectComponents.hpp>
#include <ecs/component/TransformComponents.hpp>
#include <ecs/system/time2pixel/TimePixelConverter.hpp>
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
        const auto& track_count =
            info->editorInfo.map->base_metadata().track_count;
        const auto& track_layout = info->editorInfo.track_layout;
        const auto& single_track_width = track_layout.z / float(track_count);

        auto view = registry.view<EffectComponent, TrackIdentifierComponent>();
        for (auto entity : view) {
            const auto& [effect, track_id] =
                view.get<EffectComponent, TrackIdentifierComponent>(entity);

            // 如果特效是静默状态，则跳过
            if (effect.texture_type == EffectTextureType::NONE) {
                continue;
            }
            auto effectdir = skin.nomal_hit_effect_dir;
            auto effectframes = skin.nomal_hit_effect_frame_count;
            if (effect.texture_type == EffectTextureType::SLIDE_END) {
                effectdir = skin.slide_hit_effect_dir;
                effectframes = skin.slide_hit_effect_frame_count;
            }
            auto texture_path =
                std::format("{}/{}.png", effectdir,
                            (effect.frame_index + 1) % effectframes + 1);

            auto texture = layer->get(texture_path);
            auto texsize =
                glm::vec2{single_track_width, single_track_width /
                                                  texture->origin_size.x *
                                                  texture->origin_size.y};
            texsize *= 1.25f;

            // ... (计算特效的X, Y坐标，生成渲染指令到 buffer) ...
            auto center_x = track_layout.x + single_track_width * effect.track +
                            single_track_width / 2.f;
            auto center_y = info->baseInfo.canvasSize.height() *
                            (1.f - info->editorInfo.judgeline_pos);
            PrimitiveCommand cmd;
            cmd.cmdType = CommandType::PRIMITIVE;
            cmd.primitive = PrimitiveType::QUAD;
            cmd.baseInfo.pos = {center_x - texsize.x / 2.f,
                                center_y - texsize.y / 2.f};
            cmd.baseInfo.size = texsize;
            cmd.texturesInfo.texture = texture.value();
            buffer.add_PrimitiveCommand(cmd);
        }
    }
};

#endif  // MMM_EFFECTRENDERSYSTEM_HPP
