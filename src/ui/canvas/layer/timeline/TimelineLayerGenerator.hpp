#ifndef MMM_TIMELINELAYERGENERATOR_HPP
#define MMM_TIMELINELAYERGENERATOR_HPP

#include <ecs/system/TimeLineSystem.hpp>
#include <layer/LayerComputerBase.hpp>

class TimelineLayerGenerator : public LayerComputerBase {
   public:
    using LayerComputerBase::LayerComputerBase;
    // 析构TimelineLayerGenerator
    ~TimelineLayerGenerator() override;

   protected:
    // 生成交互层的数据
    void generateLayer(LayerManager* manager,
                       RenderDataBuffer& buffer) override;

   private:
    TimeLineSystem timeline_system;
};
#endif  // MMM_TIMELINELAYERGENERATOR_HPP
