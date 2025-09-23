#ifndef MMM_INTERACTLAYERGENERATOR_HPP
#define MMM_INTERACTLAYERGENERATOR_HPP

#include <deque>
#include <ecs/system/InteractSystem.hpp>
#include <layer/LayerComputerBase.hpp>

class InteractLayerGenerator : public LayerComputerBase {
   public:
    // 构造InteractLayerGenerator
    InteractLayerGenerator(LayerManager* manager, ILayer* layer,
                           FrameSynchronizer* sync,
                           ToolInteractionState* toolinteractionstate,
                           QObject* parent = nullptr)
        : LayerComputerBase(manager, layer, sync, parent),
          tool_interaction_state(toolinteractionstate) {}

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
    ToolInteractionState* tool_interaction_state;
};

#endif  // MMM_INTERACTLAYERGENERATOR_HPP
