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

    MeshCommand cmd;
    cmd.cmdType = CommandType::MESH;
    cmd.vertices.emplace_back(glm::vec2{20, 20}, glm::vec2{0},
                              glm::vec4{1, 0, 0, 1});
    cmd.vertices.emplace_back(glm::vec2{120, 20}, glm::vec2{0},
                              glm::vec4{0, 1, 0, 1});
    cmd.vertices.emplace_back(glm::vec2{20, 120}, glm::vec2{0},
                              glm::vec4{0, 0, 1, 1});
    cmd.indicies.emplace_back(0);
    cmd.indicies.emplace_back(1);
    cmd.indicies.emplace_back(2);
    renderer->commit(cmd);

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
