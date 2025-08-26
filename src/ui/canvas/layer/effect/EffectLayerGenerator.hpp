#ifndef MMM_EFFECTLAYERGENERATOR_HPP
#define MMM_EFFECTLAYERGENERATOR_HPP

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
};

#endif  // MMM_EFFECTLAYERGENERATOR_HPP
