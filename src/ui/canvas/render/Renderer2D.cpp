#include <QFile>
#include <QMatrix4x4>
#include <QOpenGLFunctions_4_1_Core>
#include <canvas/GLCanvas.hpp>
#include <canvas/render/Renderer2D.hpp>
#include <mutex>
#include <queue>
#include <render/RenderCommand.hpp>
#include <render/texture/TexturePool.hpp>
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
    shader_program = new QOpenGLShaderProgram();

    // 从资源qrc加载
    QFile vert_source(":/glsl/canvas/quad_vshader.glsl.vert");
    QFile frag_source(":/glsl/canvas/quad_fshader.glsl.frag");
    // 检查文件是否成功打开
    if (!vert_source.open(QIODevice::ReadOnly | QIODevice::Text)) {
        auto errormsg = vert_source.errorString();
        auto errorstr = errormsg.toStdString();
        qDebug() << "Failed to open vertex source file:" << errorstr;
    }
    if (!frag_source.open(QIODevice::ReadOnly | QIODevice::Text)) {
        auto errormsg = frag_source.errorString();
        auto errorstr = errormsg.toStdString();
        qDebug() << "Failed to open frag source file:" << errorstr;
    }
    // 用QTextStream读取内容
    QTextStream vertin(&vert_source);
    QTextStream fragin(&frag_source);

    auto vertex_shader_qstr = vertin.readAll();
    auto fragment_shader_qstr = fragin.readAll();

    // 关闭文件
    vert_source.close();
    frag_source.close();

    if (!shader_program->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                                 vertex_shader_qstr)) {
        qCritical() << "Renderer Vertex Shader compilation failed:"
                    << shader_program->log();
    }
    if (!shader_program->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                                 fragment_shader_qstr)) {
        qCritical() << "Renderer Fragment Shader compilation failed:"
                    << shader_program->log();
    }
    if (!shader_program->link()) {
        qCritical() << "Shader link failed:" << shader_program->log();
    }

    // 检查是否找到了（如果拼写错误或被优化掉，可能找不到）
    if (GLuint mask_ubo_index =
            GLCALL(cvs->glGetUniformBlockIndex(shader_program->programId(),
                                               "MaskStackUBO"),
                   cvs);
        mask_ubo_index != GL_INVALID_INDEX) {
        // 将 uniform block 索引，绑定到绑定点 0
        GLCALL(cvs->glUniformBlockBinding(shader_program->programId(),
                                          mask_ubo_index, 0),
               cvs);
    } else {
        qWarning()
            << "Could not find uniform block 'MaskStackUBO' in shader program.";
    }

    // 初始化VAO
    GLCALL(cvs->glGenVertexArrays(1, &instance_dataAO), cvs);
    // 绑定VAO
    GLCALL(cvs->glBindVertexArray(instance_dataAO), cvs);

    // 初始化ubo
    GLCALL(cvs->glGenBuffers(1, &mask_uBO), cvs);
    // 绑定ubo
    GLCALL(cvs->glBindBuffer(GL_UNIFORM_BUFFER, mask_uBO), cvs);
    // 将UBO缓冲对象，也连接到绑定点 0
    // 这一步确保了绑定点0实际连接的是我们创建的 m_mask_ubo 这个GPU缓冲区
    GLCALL(cvs->glBindBufferBase(GL_UNIFORM_BUFFER, 0, mask_uBO), cvs);
    GLCALL(cvs->glBindBuffer(GL_UNIFORM_BUFFER, 0), cvs);

    // 初始化实例缓冲区
    GLCALL(cvs->glGenBuffers(1, &instance_dataBO), cvs);
    // 绑定VBO
    GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, instance_dataBO), cvs);
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

    update_attribptrFromInstance(0);

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
        shader_program->bind();
        shader_program->setUniformValue("projection", projection);
        shader_program->release();
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
        shader_program->bind();
        shader_program->setUniformValue("u_ActiveMaskLayerCount",
                                        int(mask_stack_cpu.size()));
        shader_program->release();
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

    while (max_quadcount < command_list.size()) {
        max_quadcount *= 2;
        need_update = true;
    }

    if (need_update) {
        // 绑定现有的VBO
        GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, instance_dataBO), cvs);
        // 直接用 glBufferData 重新分配，驱动会处理好旧内存的释放
        GLCALL(
            cvs->glBufferData(GL_ARRAY_BUFFER, max_quadcount * sizeof(QuadData),
                              nullptr, GL_DYNAMIC_DRAW),
            cvs);
        // 解绑
        GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, 0), cvs);
    }
}

// 提交渲染指令
void Renderer2D::commit(const RenderCommand& command) {
    {
        std::lock_guard<std::mutex> lock(command_mtx);
        command_list.push_back(command);
        // 填充data
        quad_datas.push_back(command.to_data());
    }
}

// 从指定实例位置开始更新顶点数组指针
void Renderer2D::update_attribptrFromInstance(size_t instance_index) {
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

// 结束绘制指令提交
void Renderer2D::finalize() {
    // 这个函数现在只负责分析，不再与OpenGL交互或修改command_queue
    std::lock_guard<std::mutex> lock(command_mtx);

    expandQuadDataBuffer();

    command_batch.clear();

    if (quad_datas.empty()) {
        std::queue<RenderCommand> empty_q;
        // 清理原始命令队列
        command_list.clear();
        return;
    }

    // 从第一个指令开始创建第一个批次
    command_batch.emplace_back(
        command_list.front().texturesInfo.texture.gl_texture_array_id, 0, 1);

    // 从第二个指令开始遍历
    for (size_t i = 1; i < command_list.size();
         ++i) {  // [修正] 遍历 command_list
        const auto& command = command_list[i];
        // 检查当前指令是否可以合并到最后一个批次中
        if (command.texturesInfo.texture.gl_texture_array_id ==
                command_batch.back().texture_array_id ||
            // 无纹理也可以合并
            command.texturesInfo.texture.layer_index == -1) {
            command_batch.back().instanceCount++;
        } else {
            // 不可合并，创建一个新的批次
            command_batch.emplace_back(
                command.texturesInfo.texture.gl_texture_array_id, i, 1);
        }
    }

    // 清理原始的命令队列
    command_list.clear();
}

// 渲染
void Renderer2D::render() {
    update();

    if (quad_datas.empty() || command_batch.empty()) {
        quad_datas.clear();
        return;
    }

    // 绑定核心对象
    shader_program->bind();
    GLCALL(cvs->glBindVertexArray(instance_dataAO), cvs);
    GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, instance_dataBO), cvs);

    // 一次性上传所有实例数据
    GLCALL(
        cvs->glBufferData(GL_ARRAY_BUFFER, quad_datas.size() * sizeof(QuadData),
                          quad_datas.data(), GL_DYNAMIC_DRAW),
        cvs);

    // 不绘制线框
    shader_program->setUniformValue("u_IsDrawingWireframe", false);
    // 循环遍历批处理，分批绘制
    for (const auto& batch : command_batch) {
        // 绑定这个批次需要的纹理
        // 激活纹理单元0
        GLCALL(cvs->glActiveTexture(GL_TEXTURE0), cvs);
        GLCALL(cvs->glBindTexture(GL_TEXTURE_2D_ARRAY, batch.texture_array_id),
               cvs);
        // 着色器采样器 u_samplerarray 使用纹理单元 0
        shader_program->setUniformValue("u_samplerarray", 0);

        // 更新顶点属性指针以指向当前批次的开头
        update_attribptrFromInstance(batch.startIndex);

        // 发起绘制调用
        GLCALL(
            cvs->glDrawArraysInstanced(GL_TRIANGLES, 0, 6, batch.instanceCount),
            cvs);
    }

    // === 绘制调试线框 ===
    if (draw_wireframe) {
        // 绘制线框
        shader_program->setUniformValue("u_IsDrawingWireframe", true);
        for (const auto& batch : command_batch) {
            update_attribptrFromInstance(batch.startIndex);
            GLCALL(cvs->glDrawArraysInstanced(GL_LINE_LOOP, 0, 6,
                                              batch.instanceCount),
                   cvs);
        }
    }

    // 清理
    GLCALL(cvs->glBindVertexArray(0), cvs);
    GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, 0), cvs);
    shader_program->release();

    // 为下一帧做准备
    quad_datas.clear();
    command_batch.clear();
}
