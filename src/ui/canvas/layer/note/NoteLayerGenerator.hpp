#ifndef MMM_NOTELAYERGENERATOR_HPP
#define MMM_NOTELAYERGENERATOR_HPP

#include <ecs/system/MeshGenerateSystem.hpp>
#include <ecs/system/NormalRenderSystem.hpp>
#include <ecs/system/TimeSystem.hpp>
#include <ecs/system/ToolSystem.hpp>
#include <entt.hpp>
#include <layer/LayerComputerBase.hpp>

class NoteLayerGenerator : public LayerComputerBase {
   public:
    NoteLayerGenerator(entt::registry& reg, LayerManager* manager,
                       ILayer* layer, FrameSynchronizer* sync,
                       QObject* parent = nullptr)
        : LayerComputerBase(manager, layer, sync, parent), tool_system(reg) {}
    // 析构NoteLayerGenerator
    ~NoteLayerGenerator() override;

    // 获取工具系统指针
    const ToolSystem* const get_tool_system() const { return &tool_system; }

   protected:
    // 生成交互层的数据
    void generateLayer(LayerManager* manager,
                       RenderDataBuffer& buffer) override;

   private:
    [[no_unique_address]] MeshGenerateSystem mesh_system;
    ToolSystem tool_system;
    [[no_unique_address]] NormalRenderSystem normalRender_system;
};
#endif  // MMM_NOTELAYERGENERATOR_HPP
