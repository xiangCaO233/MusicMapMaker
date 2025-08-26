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
    layer_manager->swapBuffers();
    renderer->finalize();
    renderer->render();
}

// 收集图元渲染指令
void MPrimitiveCollector::collect() {
    // 按顺序遍历图层收集渲染指令
    auto render = renderer;
    layer_manager->consume([render](const RenderDataBuffer& buffer) {
        for (auto& command_handle : buffer.all_command_handles) {
            // 提交到渲染器
            switch (command_handle.type) {
                using enum CommandType;
                case QUAD: {
                    render->commit(
                        buffer.quad_command_list[command_handle.index_in_pool]);
                    break;
                }
                case MESH: {
                    // 提交到渲染器
                    render->commit(
                        buffer.mesh_command_list[command_handle.index_in_pool]);
                    break;
                }
            }
        }
    });
}
