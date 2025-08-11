#ifndef MMM_INTERACTLAYERGENERATOR_HPP
#define MMM_INTERACTLAYERGENERATOR_HPP

#include <layer/LayerComputerBase.hpp>

class InteractLayerGenerator : public LayerComputerBase {
   public:
    // 构造InteractLayerGenerator
    using LayerComputerBase::LayerComputerBase;
    // 析构InteractLayerGenerator
    ~InteractLayerGenerator() override;

   protected:
    // 生成交互层的数据
    void generateLayer(ILayer::RenderDataBuffer& buffer) override;
};

#endif  // MMM_INTERACTLAYERGENERATOR_HPP
