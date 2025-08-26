#include <QFile>
#include <QMatrix4x4>
#include <QOpenGLFunctions_4_1_Core>
#include <canvas/GLCanvas.hpp>
#include <canvas/render/Renderer2D.hpp>
#include <mutex>
#include <render/RenderCommand.hpp>
#include <render/texture/TexturePool.hpp>
#include <type_traits>
#include <utility>

#include "render/GPUData.hpp"

template <typename Func>
auto glCallImpl(Func func, const char* funcStr,
                QOpenGLFunctions_4_1_Core* glf) {
    // 1. 先清除所有历史错误，确保我们只捕获当前调用的错误
    while (glf->glGetError() != GL_NO_ERROR);

    // 2. 对 lambda 本身的返回类型进行判断
    if constexpr (std::is_void_v<decltype(func())>) {
        func();  // 调用 lambda
    } else {
        auto&& result = func();  // 调用并捕获结果
        // 检查错误在调用之后
        if (GLenum error = glf->glGetError(); error != GL_NO_ERROR) {
            qDebug() << "OpenGL Error in [" << funcStr << "]: " << error
                     << "(Hex: 0x" << Qt::hex << error << Qt::dec << ")";
        }
        return std::forward<decltype(result)>(result);
    }

    // 针对void返回类型的lambda，在调用后检查错误
    if (GLenum error = glf->glGetError(); error != GL_NO_ERROR) {
        qDebug() << "OpenGL Error in [" << funcStr << "]: " << error
                 << "(Hex: 0x" << Qt::hex << error << Qt::dec << ")";
    }
}

// 用于包装 OpenGL 调用并检查错误
#define GLCALL(func, f) glCallImpl([&]() { return func; }, #func, f)

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

Renderer2D::Renderer2D(GLCanvas* canvas) : cvs(canvas) {
    // 初始化纹理池
    texturepool = std::make_unique<TexturePool>(canvas);

    // 初始化字体池
    fontpool = std::make_unique<FontPool>(canvas);

    // 初始化着色器
    quad_shader_program = new QOpenGLShaderProgram();
    mesh_shader_program = new QOpenGLShaderProgram();

    // 从资源qrc加载
    QFile quad_vert_source(":/glsl/canvas/quad_vshader.glsl.vert");
    QFile quad_frag_source(":/glsl/canvas/quad_fshader.glsl.frag");

    QFile mesh_vert_source(":/glsl/canvas/custom_meshvshader.glsl.vert");
    QFile mesh_frag_source(":/glsl/canvas/custom_meshfshader.glsl.frag");
    // 检查文件是否成功打开
    if (!quad_vert_source.open(QIODevice::ReadOnly | QIODevice::Text)) {
        auto errormsg = quad_vert_source.errorString();
        auto errorstr = errormsg.toStdString();
        qDebug() << "Failed to open quad vertex source file:" << errorstr;
    }
    if (!quad_frag_source.open(QIODevice::ReadOnly | QIODevice::Text)) {
        auto errormsg = quad_frag_source.errorString();
        auto errorstr = errormsg.toStdString();
        qDebug() << "Failed to open quad frag source file:" << errorstr;
    }
    // 检查文件是否成功打开
    if (!mesh_vert_source.open(QIODevice::ReadOnly | QIODevice::Text)) {
        auto errormsg = mesh_vert_source.errorString();
        auto errorstr = errormsg.toStdString();
        qDebug() << "Failed to open mesh vertex source file:" << errorstr;
    }
    if (!mesh_frag_source.open(QIODevice::ReadOnly | QIODevice::Text)) {
        auto errormsg = mesh_frag_source.errorString();
        auto errorstr = errormsg.toStdString();
        qDebug() << "Failed to open mesh frag source file:" << errorstr;
    }

    // 用QTextStream读取内容
    QTextStream quadvertin(&quad_vert_source);
    QTextStream quadfragin(&quad_frag_source);
    QTextStream meshvertin(&mesh_vert_source);
    QTextStream meshfragin(&mesh_frag_source);

    auto quad_vertex_shader_qstr = quadvertin.readAll();
    auto quad_fragment_shader_qstr = quadfragin.readAll();
    auto mesh_vertex_shader_qstr = meshvertin.readAll();
    auto mesh_fragment_shader_qstr = meshfragin.readAll();

    // 关闭文件
    quad_vert_source.close();
    quad_frag_source.close();
    mesh_vert_source.close();
    mesh_frag_source.close();

    if (!quad_shader_program->addShaderFromSourceCode(
            QOpenGLShader::Vertex, quad_vertex_shader_qstr)) {
        qCritical() << "Renderer Quad Vertex Shader compilation failed:"
                    << quad_shader_program->log();
    }
    if (!quad_shader_program->addShaderFromSourceCode(
            QOpenGLShader::Fragment, quad_fragment_shader_qstr)) {
        qCritical() << "Renderer Quad Fragment Shader compilation failed:"
                    << quad_shader_program->log();
    }
    if (!quad_shader_program->link()) {
        qCritical() << "QuadShader link failed:" << quad_shader_program->log();
    }

    if (!mesh_shader_program->addShaderFromSourceCode(
            QOpenGLShader::Vertex, mesh_vertex_shader_qstr)) {
        qCritical() << "Renderer Mesh Vertex Shader compilation failed:"
                    << mesh_shader_program->log();
    }
    if (!mesh_shader_program->addShaderFromSourceCode(
            QOpenGLShader::Fragment, mesh_fragment_shader_qstr)) {
        qCritical() << "Renderer Mesh Fragment Shader compilation failed:"
                    << mesh_shader_program->log();
    }
    if (!mesh_shader_program->link()) {
        qCritical() << "MeshShader link failed:" << mesh_shader_program->log();
    }

    // 检查是否找到了（如果拼写错误或被优化掉，可能找不到）
    if (GLuint mask_ubo_index =
            GLCALL(cvs->glGetUniformBlockIndex(quad_shader_program->programId(),
                                               "MaskStackUBO"),
                   cvs);
        mask_ubo_index != GL_INVALID_INDEX) {
        // 将 uniform block 索引，绑定到绑定点 0
        GLCALL(cvs->glUniformBlockBinding(quad_shader_program->programId(),
                                          mask_ubo_index, 0),
               cvs);
    } else {
        qWarning()
            << "Could not find uniform block 'MaskStackUBO' in shader program.";
    }

    // quadshader
    // 初始化VAO
    GLCALL(cvs->glGenVertexArrays(1, &quad_instance_dataAO), cvs);
    // 绑定VAO
    GLCALL(cvs->glBindVertexArray(quad_instance_dataAO), cvs);

    // 初始化ubo
    GLCALL(cvs->glGenBuffers(1, &mask_uBO), cvs);
    // 绑定ubo
    GLCALL(cvs->glBindBuffer(GL_UNIFORM_BUFFER, mask_uBO), cvs);
    // 将UBO缓冲对象，也连接到绑定点 0
    // 这一步确保了绑定点0实际连接的是我们创建的 m_mask_ubo 这个GPU缓冲区
    GLCALL(cvs->glBindBufferBase(GL_UNIFORM_BUFFER, 0, mask_uBO), cvs);
    GLCALL(cvs->glBindBuffer(GL_UNIFORM_BUFFER, 0), cvs);

    // 初始化实例缓冲区
    GLCALL(cvs->glGenBuffers(1, &quad_instance_dataBO), cvs);
    // 绑定VBO
    GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, quad_instance_dataBO), cvs);
    GLCALL(cvs->glBufferData(GL_ARRAY_BUFFER, max_quadcount * sizeof(QuadData),
                             nullptr, GL_DYNAMIC_DRAW),
           cvs);

    // 0~2 vec2 pos
    GLCALL(cvs->glEnableVertexAttribArray(0), cvs);

    // 3~4 vec2 size
    GLCALL(cvs->glEnableVertexAttribArray(1), cvs);

    // 5 f32 rotation
    GLCALL(cvs->glEnableVertexAttribArray(2), cvs);

    // 6~9 vec4 color
    GLCALL(cvs->glEnableVertexAttribArray(3), cvs);

    // 10~11 vec2 radius
    GLCALL(cvs->glEnableVertexAttribArray(4), cvs);

    // 12 f32 radius_effect_param
    GLCALL(cvs->glEnableVertexAttribArray(5), cvs);

    // 13 uint radius_effect
    GLCALL(cvs->glEnableVertexAttribArray(6), cvs);

    // 14~15 vec2 uv_scale
    GLCALL(cvs->glEnableVertexAttribArray(7), cvs);

    // 16~17 vec2 group_size
    GLCALL(cvs->glEnableVertexAttribArray(8), cvs);

    // 18 uint no_filter
    GLCALL(cvs->glEnableVertexAttribArray(9), cvs);

    // 19 int layer_idx
    GLCALL(cvs->glEnableVertexAttribArray(10), cvs);

    // 20 uint texalignmode
    GLCALL(cvs->glEnableVertexAttribArray(11), cvs);

    // 21 uint texscalemode
    GLCALL(cvs->glEnableVertexAttribArray(12), cvs);

    updateQuadAttribptrFromInstance(0);

    GLCALL(cvs->glVertexAttribDivisor(0, 1), cvs);   // pos
    GLCALL(cvs->glVertexAttribDivisor(1, 1), cvs);   // size
    GLCALL(cvs->glVertexAttribDivisor(2, 1), cvs);   // rotation
    GLCALL(cvs->glVertexAttribDivisor(3, 1), cvs);   // color
    GLCALL(cvs->glVertexAttribDivisor(4, 1), cvs);   // radius
    GLCALL(cvs->glVertexAttribDivisor(5, 1), cvs);   // radius_effect_param
    GLCALL(cvs->glVertexAttribDivisor(6, 1), cvs);   // radius_effect
    GLCALL(cvs->glVertexAttribDivisor(7, 1), cvs);   // uv_scale
    GLCALL(cvs->glVertexAttribDivisor(8, 1), cvs);   // group_size
    GLCALL(cvs->glVertexAttribDivisor(9, 1), cvs);   // layer_idx
    GLCALL(cvs->glVertexAttribDivisor(10, 1), cvs);  // no_filter
    GLCALL(cvs->glVertexAttribDivisor(11, 1), cvs);  // talign
    GLCALL(cvs->glVertexAttribDivisor(12, 1), cvs);  // tscale

    // 解绑
    GLCALL(cvs->glBindVertexArray(0), cvs);
    GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, 0), cvs);
}

Renderer2D::~Renderer2D() {
    // 释放纹理池
    texturepool.reset();
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

// 更新需要更新的资源等等
void Renderer2D::update() {
    if (texturepool->needupdate().load()) {
        texturepool->processUpdateDirRequest();
        texturepool->processUploadQueue();
        // 发送需更新皮肤信息信号
        emit needUpdateTexinfo();
    }
    if (fontpool->needupdate().load()) {
        fontpool->processUploadQueue();
        // 发送需更新皮肤信息信号
        emit needUpdateTexinfo();
    }

    if (update_view) {
        QMatrix4x4 projection;
        projection.ortho(0.0f, viewport.x, viewport.y, 0.0f, -1.0f, 1.0f);
        quad_shader_program->bind();
        quad_shader_program->setUniformValue("projection", projection);
        quad_shader_program->release();
        update_view = false;
    }

    if (update_ubo) {
        // 更新ubo
        // 将CPU端的蒙版堆栈数据上传到UBO
        GLCALL(cvs->glBindBuffer(GL_UNIFORM_BUFFER, mask_uBO), cvs);
        GLCALL(cvs->glBufferData(GL_UNIFORM_BUFFER,
                                 MAX_MASK_LAYERS * sizeof(MaskLayer_STD140),
                                 mask_stack_cpu.data(), GL_STATIC_DRAW),
               cvs);
        GLCALL(cvs->glBindBuffer(GL_UNIFORM_BUFFER, 0), cvs);

        // 需要一个uniform告诉着色器当前有多少个活跃的蒙版层
        quad_shader_program->bind();
        quad_shader_program->setUniformValue("u_ActiveMaskLayerCount",
                                             int(mask_stack_cpu.size()));
        quad_shader_program->release();
        update_ubo = false;
    }
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

// 扩充矩形实例缓冲区
void Renderer2D::expandQuadDataBuffer() {
    bool need_update{false};

    while (max_quadcount < quad_command_list.size()) {
        max_quadcount *= 2;
        need_update = true;
    }

    if (need_update) {
        // 绑定矩形实例VBO
        GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, quad_instance_dataBO), cvs);
        // 直接用 glBufferData 重新分配，驱动会处理好旧内存的释放
        GLCALL(
            cvs->glBufferData(GL_ARRAY_BUFFER, max_quadcount * sizeof(QuadData),
                              nullptr, GL_DYNAMIC_DRAW),
            cvs);
        // 解绑
        GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, 0), cvs);
    }
}
// 扩充网格缓冲区
void Renderer2D::expandMeshDataBuffer() {
    bool need_update{false};
    auto vertsize{0};
    for (const auto& meshdata : mesh_datas) {
        vertsize += meshdata.vertices.size();
    }
    while (max_mesh_vertexcount < vertsize) {
        max_mesh_vertexcount *= 2;
        need_update = true;
    }

    if (need_update) {
        // 绑定网格VBO
        GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, mesh_dataBO), cvs);
        GLCALL(cvs->glBufferData(GL_ARRAY_BUFFER,
                                 max_mesh_vertexcount * sizeof(CustomVertex),
                                 nullptr, GL_DYNAMIC_DRAW),
               cvs);
        // 解绑
        GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, 0), cvs);
    }
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

// 从指定矩形实例位置开始更新顶点数组指针
void Renderer2D::updateQuadAttribptrFromInstance(size_t instance_index) const {
    // 计算当前实例索引在VBO中的字节偏移量
    size_t base_offset = instance_index * sizeof(QuadData);

    // 0~2 vec2 pos
    GLCALL(cvs->glVertexAttribPointer(
               0, 2, GL_FLOAT, false, sizeof(QuadData),
               (void*)(base_offset + offsetof(QuadData, pos))),
           cvs);

    // 3~4 vec2 size
    GLCALL(cvs->glVertexAttribPointer(
               1, 2, GL_FLOAT, false, sizeof(QuadData),
               (void*)(base_offset + offsetof(QuadData, size))),
           cvs);

    // 5 f32 rotation
    GLCALL(cvs->glVertexAttribPointer(
               2, 1, GL_FLOAT, false, sizeof(QuadData),
               (void*)(base_offset + offsetof(QuadData, rotation))),
           cvs);

    // 6~9 vec4 color
    GLCALL(cvs->glVertexAttribPointer(
               3, 4, GL_FLOAT, false, sizeof(QuadData),
               (void*)(base_offset + offsetof(QuadData, color))),
           cvs);

    // 10~11 vec2 radius
    GLCALL(cvs->glVertexAttribPointer(
               4, 2, GL_FLOAT, false, sizeof(QuadData),
               (void*)(base_offset + offsetof(QuadData, radius))),
           cvs);

    // 12 f32 radius_effect_param
    GLCALL(cvs->glVertexAttribPointer(
               5, 1, GL_FLOAT, false, sizeof(QuadData),
               (void*)(base_offset + offsetof(QuadData, radius_effect_param))),
           cvs);

    // 13 uint radius_effect
    GLCALL(cvs->glVertexAttribIPointer(
               6, 1, GL_UNSIGNED_INT, sizeof(QuadData),
               (void*)(base_offset + offsetof(QuadData, radius_effect))),
           cvs);

    // 14~15 vec2 uv_scale
    GLCALL(cvs->glVertexAttribPointer(
               7, 2, GL_FLOAT, false, sizeof(QuadData),
               (void*)(base_offset + offsetof(QuadData, uv_scale))),
           cvs);

    // 16~17 vec2 group_size
    GLCALL(cvs->glVertexAttribPointer(
               8, 2, GL_FLOAT, false, sizeof(QuadData),
               (void*)(base_offset + offsetof(QuadData, group_size))),
           cvs);

    // 18 int layer_idx
    GLCALL(cvs->glVertexAttribIPointer(
               9, 1, GL_INT, sizeof(QuadData),
               (void*)(base_offset + offsetof(QuadData, layer_idx))),
           cvs);

    // 19 uint no_filter
    GLCALL(cvs->glVertexAttribIPointer(
               10, 1, GL_UNSIGNED_INT, sizeof(QuadData),
               (void*)(base_offset + offsetof(QuadData, no_filter))),
           cvs);

    // 20 uint texalignmode
    GLCALL(cvs->glVertexAttribIPointer(
               11, 1, GL_UNSIGNED_INT, sizeof(QuadData),
               (void*)(base_offset + offsetof(QuadData, talign))),
           cvs);

    // 21 uint texscalemode
    GLCALL(cvs->glVertexAttribIPointer(
               12, 1, GL_UNSIGNED_INT, sizeof(QuadData),
               (void*)(base_offset + offsetof(QuadData, tscale))),
           cvs);
}

// 从指定顶点位置开始更新网格顶点数组指针
void Renderer2D::updateMeshAttribptrFromInstance(size_t vertex_index) const {}

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

void Renderer2D::drawBatchFirst(const RenderBatch& batch) const {
    switch (batch.type) {
        using enum CommandType;
        case QUAD: {
            // 更新矩形实例数组指针
            updateQuadAttribptrFromInstance(batch.startIndex);
            GLCALL(cvs->glDrawArraysInstanced(GL_TRIANGLES, 0, 6,
                                              batch.elementCount),
                   cvs);
            break;
        }
        case MESH: {
            // 更新网格数组指针
            updateMeshAttribptrFromInstance(batch.startIndex);
            GLCALL(cvs->glDrawArrays(GL_TRIANGLES, 0, batch.elementCount), cvs);
            break;
        }
    }
}

void Renderer2D::drawWireframeBatchNext(const RenderBatch& batch) const {
    switch (batch.type) {
        using enum CommandType;
        case QUAD: {
            GLCALL(cvs->glDrawArraysInstanced(GL_LINE_LOOP, 0, 6,
                                              batch.elementCount),
                   cvs);
            break;
        }
        case MESH: {
            GLCALL(cvs->glDrawArrays(GL_LINE_LOOP, 0, batch.elementCount), cvs);
            break;
        }
    }
}

// 渲染
void Renderer2D::render() {
    update();

    if (command_batchs.empty()) {
        quad_datas.clear();
        mesh_datas.clear();
        return;
    }

    // 更新gpu数据
    // === 1. 一次性上传所有GPU数据 ===
    if (!quad_datas.empty()) {
        // 绑定矩形实例缓冲区
        GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, quad_instance_dataBO), cvs);
        // 一次性上传所有矩形实例数据
        GLCALL(cvs->glBufferData(GL_ARRAY_BUFFER,
                                 quad_datas.size() * sizeof(QuadData),
                                 quad_datas.data(), GL_DYNAMIC_DRAW),
               cvs);
    }
    if (!mesh_datas.empty()) {
        // 绑定网格缓冲区
        GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, mesh_dataBO), cvs);
        // 一次性上传所有网格数据
        GLCALL(cvs->glBufferData(GL_ARRAY_BUFFER,
                                 mesh_datas.size() * sizeof(MeshData),
                                 mesh_datas.data(), GL_DYNAMIC_DRAW),
               cvs);
    }

    // === 2. 在单个循环中通过状态追踪进行渲染 ===
    QOpenGLShaderProgram* current_shader{nullptr};
    uint32_t current_vao{0};

    for (const auto& batch : command_batchs) {
        // 确定此批次所需的状态

        // 获取需要的gpu实例
        auto required_shader = useShader(batch.type);
        auto required_vao = useVAO(batch.type);

        // --- 状态切换逻辑 ---
        if (current_shader != required_shader) {
            if (current_shader) {
                current_shader->release();
            }
            current_shader = required_shader;
            current_shader->bind();
        }
        if (current_vao != required_vao) {
            current_vao = required_vao;
            GLCALL(cvs->glBindVertexArray(current_vao), cvs);
        }

        // 设置通用状态 (纹理)
        GLCALL(cvs->glActiveTexture(GL_TEXTURE0), cvs);
        GLCALL(cvs->glBindTexture(GL_TEXTURE_2D_ARRAY, batch.texture_array_id),
               cvs);
        current_shader->setUniformValue("u_samplerarray", 0);

        // --- 发起主绘制调用 ---
        drawBatchFirst(batch);

        // 发起可选的线框绘制调用
        if (draw_wireframe) {
            drawWireframeBatchNext(batch);
        }
    }

    // === 3. 最终清理 ===
    GLCALL(cvs->glBindVertexArray(0), cvs);
    if (current_shader) {
        current_shader->release();
    }

    // 为下一帧做准备
    quad_datas.clear();
    mesh_datas.clear();
    command_batchs.clear();
}
