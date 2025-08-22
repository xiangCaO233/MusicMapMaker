#ifndef MMM_NOTELAYERGENERATOR_HPP
#define MMM_NOTELAYERGENERATOR_HPP

#include <ecs/system/MeshGenerateSystem.hpp>
#include <ecs/system/TimeSystem.hpp>
#include <layer/LayerComputerBase.hpp>

#include "ecs/system/NormalRenderSystem.hpp"

class NoteLayerGenerator : public LayerComputerBase {
   public:
    using LayerComputerBase::LayerComputerBase;
    // 析构NoteLayerGenerator
    ~NoteLayerGenerator() override;

   protected:
    // 生成交互层的数据
    void generateLayer(LayerManager* manager,
                       ILayer::RenderDataBuffer& buffer) override;

   private:
    [[no_unique_address]] MeshGenerateSystem mesh_system;
    [[no_unique_address]] NormalRenderSystem normalRender_system;
};
#endif  // MMM_NOTELAYERGENERATOR_HPP
