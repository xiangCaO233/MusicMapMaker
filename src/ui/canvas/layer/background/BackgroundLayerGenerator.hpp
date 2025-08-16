#ifndef MMM_BACKGROUNDLAYERGENERATOR_HPP
#define MMM_BACKGROUNDLAYERGENERATOR_HPP

#include <layer/LayerComputerBase.hpp>

class BackgroundLayerGenerator : public LayerComputerBase {
   public:
    using LayerComputerBase::LayerComputerBase;
    // 析构BackgroundLayerGenerator
    ~BackgroundLayerGenerator() override;

   protected:
    // 生成图层
    void generateLayer(LayerManager* manager,
                       ILayer::RenderDataBuffer& buffer) override;
};
#endif  // MMM_BACKGROUNDLAYERGENERATOR_HPP
