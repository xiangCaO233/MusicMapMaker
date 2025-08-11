#include <layer/LayerManager.hpp>
#include <render/MPrimitiveCollector.hpp>
#include <render/Renderer2D.hpp>

// 构造MPrimitiveCollector
MPrimitiveCollector::MPrimitiveCollector(Renderer2D* render,
                                         LayerManager* manager)
    : renderer(render), layer_manager(manager) {}

// 析构MPrimitiveCollector
MPrimitiveCollector::~MPrimitiveCollector() {
    collect();
    renderer->finalize();
    renderer->render();
}

// 收集图元渲染指令
void MPrimitiveCollector::collect() {
    // 按顺序遍历图层收集渲染指令
    auto render = renderer;
    layer_manager->consume([render](const ILayer::RenderDataBuffer& buffer) {
        for (auto& command : buffer) {
            // 提交到渲染器
            render->commit(command);
        }
    });
}
