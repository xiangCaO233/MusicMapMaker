#ifndef MMM_NOTELAYERGENERATOR_HPP
#define MMM_NOTELAYERGENERATOR_HPP

#include <ecs/system/NormalRenderSystem.hpp>
#include <ecs/system/TimeSystem.hpp>
#include <ecs/system/ToolSystem.hpp>
#include <ecs/system/mesh/MeshGenerateSystem.hpp>
#include <entt.hpp>
#include <layer/LayerComputerBase.hpp>

class NoteLayerGenerator : public LayerComputerBase {
   public:
    NoteLayerGenerator(LayerManager* manager, ILayer* layer,
                       FrameSynchronizer* sync, ToolSystem* toolsystem,
                       ToolInteractionState* toolinteractionstate,
                       QObject* parent = nullptr)
        : LayerComputerBase(manager, layer, sync, parent),
          tool_system(toolsystem),
          tool_interaction_state(toolinteractionstate) {}
    // 析构NoteLayerGenerator
    ~NoteLayerGenerator() override;

   protected:
    // 生成交互层的数据
    void generateLayer(LayerManager* manager,
                       RenderDataBuffer& buffer) override;

   private:
    [[no_unique_address]] MeshGenerateSystem mesh_system;
    [[no_unique_address]] NormalRenderSystem normalRender_system;
    ToolSystem* tool_system;
    ToolInteractionState* tool_interaction_state;
};
#endif  // MMM_NOTELAYERGENERATOR_HPP
