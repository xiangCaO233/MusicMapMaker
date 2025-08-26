#ifndef MMM_INTERACTLAYERGENERATOR_HPP
#define MMM_INTERACTLAYERGENERATOR_HPP

#include <deque>
#include <ecs/system/InteractSystem.hpp>
#include <layer/LayerComputerBase.hpp>

class InteractLayerGenerator : public LayerComputerBase {
   public:
    // 构造InteractLayerGenerator
    using LayerComputerBase::LayerComputerBase;

    InteractLayerGenerator();

    // 析构InteractLayerGenerator
    ~InteractLayerGenerator() override;
    std::deque<glm::vec2> m_pos_history;  // 用于存储鼠标历史位置的队列

   protected:
    // 生成交互层的数据
    void generateLayer(LayerManager* manager,
                       RenderDataBuffer& buffer) override;

   private:
    InteractSystem interact_system;
};

#endif  // MMM_INTERACTLAYERGENERATOR_HPP
