#include <QFile>
#include <canvas/GLCanvas.hpp>
#include <render/Renderer2D.hpp>
#include <type_traits>
#include <utility>

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

// 初始化着色器
void Renderer2D::initQuadShader() {
    // 初始化着色器
    quad_shader_program = new QOpenGLShaderProgram();
    // 从资源qrc加载
    QFile quad_vert_source(":/glsl/canvas/quad_vshader.glsl.vert");
    QFile quad_frag_source(":/glsl/canvas/quad_fshader.glsl.frag");
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
    // 用QTextStream读取内容
    QTextStream quadvertin(&quad_vert_source);
    QTextStream quadfragin(&quad_frag_source);
    auto quad_vertex_shader_qstr = quadvertin.readAll();
    auto quad_fragment_shader_qstr = quadfragin.readAll();
    // 关闭文件
    quad_vert_source.close();
    quad_frag_source.close();

    // 编译链接着色器
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

    // 检查是否找到了UBO块（如果拼写错误或被优化掉，可能找不到）
    if (GLuint quad_mask_ubo_index =
            GLCALL(cvs->glGetUniformBlockIndex(quad_shader_program->programId(),
                                               "MaskStackUBO"),
                   cvs);
        quad_mask_ubo_index != GL_INVALID_INDEX) {
        // 将 uniform block 索引，绑定到绑定点 0
        GLCALL(cvs->glUniformBlockBinding(quad_shader_program->programId(),
                                          quad_mask_ubo_index, 0),
               cvs);
    } else {
        qWarning() << "Could not find uniform block 'MaskStackUBO' in quad "
                      "shader program.";
    }
}

void Renderer2D::initQuadObjectBuffers() {
    // quadbuffer
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

void Renderer2D::initMeshShader() {
    // 初始化着色器
    mesh_shader_program = new QOpenGLShaderProgram();

    // 从资源qrc加载
    QFile mesh_vert_source(":/glsl/canvas/custom_meshvshader.glsl.vert");
    QFile mesh_frag_source(":/glsl/canvas/custom_meshfshader.glsl.frag");
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
    QTextStream meshvertin(&mesh_vert_source);
    QTextStream meshfragin(&mesh_frag_source);

    auto mesh_vertex_shader_qstr = meshvertin.readAll();
    auto mesh_fragment_shader_qstr = meshfragin.readAll();

    // 关闭文件
    mesh_vert_source.close();
    mesh_frag_source.close();

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

    // 检查是否找到了UBO块（如果拼写错误或被优化掉，可能找不到）
    if (GLuint mesh_mask_ubo_index =
            GLCALL(cvs->glGetUniformBlockIndex(mesh_shader_program->programId(),
                                               "MaskStackUBO"),
                   cvs);
        mesh_mask_ubo_index != GL_INVALID_INDEX) {
        // 将 uniform block 索引，绑定到绑定点 0
        GLCALL(cvs->glUniformBlockBinding(mesh_shader_program->programId(),
                                          mesh_mask_ubo_index, 0),
               cvs);
    } else {
        qWarning() << "Could not find uniform block 'MaskStackUBO' in mesh "
                      "shader program.";
    }
}

void Renderer2D::initMeshBuffers() {}

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

// 批绘制
void Renderer2D::drawBatch(const RenderBatch& batch, GLenum mode) const {
    switch (batch.type) {
        using enum CommandType;
        case QUAD: {
            // 更新矩形实例数组指针
            updateQuadAttribptrFromInstance(batch.startIndex);
            GLCALL(cvs->glDrawArraysInstanced(mode, 0, 6, batch.elementCount),
                   cvs);
            break;
        }
        case MESH: {
            GLCALL(
                cvs->glDrawArrays(mode, batch.startIndex, batch.elementCount),
                cvs);
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

        // --- 发起绘制调用 ---
        current_shader->setUniformValue("u_IsDrawingWireframe", false);
        drawBatch(batch, GL_TRIANGLES);  // 明确告知使用三角形模式

        // 2. 如果需要，再绘制线框模式
        if (draw_wireframe) {
            current_shader->setUniformValue("u_IsDrawingWireframe", true);
            drawBatch(batch, GL_LINE_LOOP);  // 明确告知使用线循环模式
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
