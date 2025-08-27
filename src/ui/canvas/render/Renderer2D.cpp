#include <QFile>
#include <QMatrix4x4>
#include <QOpenGLFunctions_4_1_Core>
#include <canvas/GLCanvas.hpp>
#include <canvas/render/Renderer2D.hpp>
#include <mutex>
#include <render/GPUData.hpp>
#include <render/RenderCommand.hpp>
#include <render/texture/TexturePool.hpp>

Renderer2D::Renderer2D(GLCanvas* canvas) : cvs(canvas) {
    // 初始化纹理池
    texturepool = std::make_unique<TexturePool>(canvas);

    // 初始化字体池
    fontpool = std::make_unique<FontPool>(canvas);

    initQuadShader();
    initQuadObjectBuffers();
    initMeshShader();
    initMeshBuffers();
}

Renderer2D::~Renderer2D() {
    // 释放纹理池
    texturepool.reset();
}

// 需要卸载纹理
void Renderer2D::need_unloadtexture_dir(std::string_view texdir) {
    request_remove_texture_from_path(std::string(texdir));
}

// 需要载入纹理
void Renderer2D::need_loadtexture_dir(std::string_view texdir) {
    request_texture_from_path(std::string(texdir));
}

// 获取信息
TextureInfo Renderer2D::getInfo(std::string_view texname) {
    return texturepool->get(texname).value_or(TextureInfo{});
}

const RenderCommand& Renderer2D::get_command_from_handle(
    const CommandHandle& handle) {
    switch (handle.type) {
        using enum CommandType;
        case QUAD: {
            return quad_command_list[handle.index_in_pool];
        }
        case MESH: {
            return mesh_command_list[handle.index_in_pool];
        }
        default:
            return nullCmd;
    }
}

QOpenGLShaderProgram* Renderer2D::useShader(CommandType type) {
    switch (type) {
        using enum CommandType;
        case QUAD: {
            return quad_shader_program;
        }
        case MESH: {
            return mesh_shader_program;
        }
        default:
            return nullptr;
    }
}

uint32_t Renderer2D::useVAO(CommandType type) const {
    switch (type) {
        using enum CommandType;
        case QUAD: {
            return quad_instance_dataAO;
        }
        case MESH: {
            return mesh_dataAO;
        }
        default:
            return 0;
    }
}

void Renderer2D::add_texture_from_path(const std::string& path) {
    texturepool->add_directory(path);
}

void Renderer2D::request_texture_from_path(const std::string& path) {
    texturepool->request_new_directory(path);
}

// 移除纹理目录
void Renderer2D::remove_texture_from_path(const std::string& path) {
    texturepool->remove_directory(path);
}

void Renderer2D::request_remove_texture_from_path(const std::string& path) {
    texturepool->request_remove_directory(path);
}

// 添加字体
void Renderer2D::add_font_from_path(const std::string& path, bool is_qrc) {
    fontpool->load_font(path, is_qrc);
}

void Renderer2D::update_viewport(glm::vec2 view) {
    viewport = view;
    update_ubo = true;
    update_view = true;
}

// 新建蒙版
void Renderer2D::newMask(glm::vec4 rect, glm::vec4 effectParams,
                         MaskEffect effect) {
    // 添加一个蒙版
    mask_stack_cpu.push_back({rect, effectParams, effect, {0, 0, 0}});
    update_ubo = true;
}

// 提交渲染指令
void Renderer2D::commit(const QuadCommand& command) {
    {
        std::lock_guard<std::mutex> lock(command_mtx);
        quad_command_list.push_back(command);
        // 在统一的句柄列表中记录它的类型和位置
        all_command_handles.push_back(
            {CommandType::QUAD, quad_command_list.size() - 1});
        // 填充gpu数据
        quad_datas.push_back(command.to_data());
    }
}

void Renderer2D::commit(const MeshCommand& command) {
    {
        std::lock_guard<std::mutex> lock(command_mtx);
        mesh_command_list.push_back(command);
        // 在统一的句柄列表中记录它的类型和位置
        all_command_handles.push_back(
            {CommandType::MESH, mesh_command_list.size() - 1});
        // 填充gpu数据
        mesh_datas.push_back(command.to_data());
    }
}

bool cmd_emergable(const RenderCommand& command,
                   const std::vector<RenderBatch>& batchs) {
    // 检查当前指令是否可以合并到最后一个批次中
    // 指令类型一致且使用的纹理id一致
    // 无纹理也可以合并
    const auto& batch_tail = batchs.back();
    return (command.cmdType == batch_tail.type &&
            command.texturesInfo.texture.gl_texture_array_id ==
                batch_tail.texture_array_id) ||
           command.texturesInfo.texture.gl_texture_array_id == -1;
}

// 结束绘制指令提交
void Renderer2D::finalize() {
    // 这个函数现在只负责分析，不再与OpenGL交互或修改command_queue
    std::lock_guard<std::mutex> lock(command_mtx);

    expandQuadDataBuffer();
    expandMeshDataBuffer();

    command_batchs.clear();

    if (all_command_handles.empty()) {
        return;
    }
    if (quad_datas.empty() && mesh_datas.empty()) {
        // 清理原始命令队列
        quad_command_list.clear();
        mesh_command_list.clear();
        all_command_handles.clear();
        return;
    }
    // 为不同类型的指令维护独立的实例计数器
    size_t quad_instance_counter = 0;
    size_t mesh_instance_counter = 0;

    // --- 创建第一个批次 ---
    const auto& first_handle = all_command_handles.front();
    const RenderCommand& first_cmd = get_command_from_handle(first_handle);
    command_batchs.emplace_back(
        first_cmd.cmdType, first_cmd.texturesInfo.texture.gl_texture_array_id,
        0,  // 第一个批次的 instanceStartIndex 总是 0
        1);
    // 更新相应的计数器
    if (first_cmd.cmdType == CommandType::QUAD) {
        quad_instance_counter++;
    } else {
        mesh_instance_counter++;
    }

    // --- 从第二个指令开始遍历 ---
    for (size_t i = 1; i < all_command_handles.size(); ++i) {
        const auto& handle = all_command_handles[i];
        const RenderCommand& command = get_command_from_handle(handle);

        if (cmd_emergable(command, command_batchs)) {
            // 合并到当前批次
            command_batchs.back().elementCount++;
        } else {
            // 不可合并，创建一个新的批次
            size_t new_instance_start_index = 0;
            switch (command.cmdType) {
                using enum CommandType;
                case QUAD: {
                    new_instance_start_index = quad_instance_counter;
                    break;
                }
                case MESH: {
                    new_instance_start_index = mesh_instance_counter;
                    break;
                }
            }

            command_batchs.emplace_back(
                command.cmdType,
                command.texturesInfo.texture.gl_texture_array_id,
                // ★ 使用类型特定的实例索引
                new_instance_start_index, 1);
        }

        // 无论是否合并，都必须更新计数器
        if (command.cmdType == CommandType::QUAD) {
            quad_instance_counter++;
        } else {
            mesh_instance_counter++;
        }
    }

    // 清理所有列表
    quad_command_list.clear();
    mesh_command_list.clear();
    all_command_handles.clear();
}
