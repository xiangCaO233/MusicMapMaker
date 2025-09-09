#ifndef MMM_EFFECTLAYERGENERATOR_HPP
#define MMM_EFFECTLAYERGENERATOR_HPP

#include <ecs/system/AudioEffectSystem.hpp>
#include <ecs/system/EffectRenderSystem.hpp>
#include <layer/LayerComputerBase.hpp>

class EffectLayerGenerator : public LayerComputerBase {
   public:
    using LayerComputerBase::LayerComputerBase;
    // 析构EffectLayerGenerator
    ~EffectLayerGenerator() override;

   protected:
    // 生成图层
    void generateLayer(LayerManager* manager,
                       RenderDataBuffer& buffer) override;

   private:
    [[no_unique_address]] EffectRenderSystem effect_render_system;
    [[no_unique_address]] AudioEffectSystem audio_effect_system;
};

#endif  // MMM_EFFECTLAYERGENERATOR_HPP
