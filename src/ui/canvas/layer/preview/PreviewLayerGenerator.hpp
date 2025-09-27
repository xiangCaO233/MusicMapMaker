#ifndef MMM_PREVIEWLAYERGENERATOR_HPP
#define MMM_PREVIEWLAYERGENERATOR_HPP

#include <ecs/system/RenderSystem.hpp>
#include <ecs/system/TimeLineSystem.hpp>
#include <ecs/system/TimeSystem.hpp>
#include <ecs/system/ToolSystem.hpp>
#include <ecs/system/mesh/MeshGenerateSystem.hpp>
#include <entt.hpp>
#include <layer/LayerComputerBase.hpp>

class PreviewLayerGenerator : public LayerComputerBase {
   public:
    PreviewLayerGenerator(LayerManager* manager, ILayer* layer,
                          FrameSynchronizer* sync, ToolSystem* toolsystem,
                          ToolInteractionState* toolinteractionstate,
                          QObject* parent = nullptr)
        : LayerComputerBase(manager, layer, sync, parent),
          tool_system(toolsystem),
          tool_interaction_state(toolinteractionstate) {}
    // 析构PreviewLayerGenerator
    ~PreviewLayerGenerator() override;

   protected:
    // 生成交互层的数据
    void generateLayer(LayerManager* manager,
                       RenderDataBuffer& buffer) override;

   private:
    [[no_unique_address]] MeshGenerateSystem mesh_system;
    [[no_unique_address]] RenderSystem render_system;
    [[no_unique_address]] TimeLineSystem timeline_system;
    ToolSystem* tool_system;
    ToolInteractionState* tool_interaction_state;
};
#endif  // MMM_PREVIEWLAYERGENERATOR_HPP
