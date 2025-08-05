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

Renderer2D::Renderer2D(GLCanvas* canvas) : glf(canvas) {
    // 初始化纹理池
    texturepool = std::make_unique<TexturePool>(canvas);
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

    // 初始化VAO
    GLCALL(glf->glGenVertexArrays(1, &instance_dataAO), glf);
    // 绑定VAO
    GLCALL(glf->glBindVertexArray(instance_dataAO), glf);

    GLCALL(glf->glGenBuffers(1, &instance_dataBO), glf);
    // 绑定VBO
    GLCALL(glf->glBindBuffer(GL_ARRAY_BUFFER, instance_dataBO), glf);
    GLCALL(glf->glBufferData(GL_ARRAY_BUFFER, max_quadcount * sizeof(QuadData),
                             nullptr, GL_DYNAMIC_DRAW),
           glf);

    // 0~2 vec2 pos
    GLCALL(glf->glEnableVertexAttribArray(0), glf);

    // 3~4 vec2 size
    GLCALL(glf->glEnableVertexAttribArray(1), glf);

    // 5 f32 rotation
    GLCALL(glf->glEnableVertexAttribArray(2), glf);

    // 6~9 vec4 color
    GLCALL(glf->glEnableVertexAttribArray(3), glf);

    // 10~11 vec2 uv_scale
    GLCALL(glf->glEnableVertexAttribArray(4), glf);

    // 12 uint layer_idx
    GLCALL(glf->glEnableVertexAttribArray(5), glf);

    // 13 uint texalignmode
    GLCALL(glf->glEnableVertexAttribArray(6), glf);

    // 14 uint texscalemode
    GLCALL(glf->glEnableVertexAttribArray(7), glf);

    update_attribptrFromInstance(0);

    GLCALL(glf->glVertexAttribDivisor(0, 1), glf);  // pos
    GLCALL(glf->glVertexAttribDivisor(1, 1), glf);  // size
    GLCALL(glf->glVertexAttribDivisor(2, 1), glf);  // rotation
    GLCALL(glf->glVertexAttribDivisor(3, 1), glf);  // color
    GLCALL(glf->glVertexAttribDivisor(4, 1), glf);  // uv_scale
    GLCALL(glf->glVertexAttribDivisor(5, 1), glf);  // layer_idx
    GLCALL(glf->glVertexAttribDivisor(6, 1), glf);  // talign
    GLCALL(glf->glVertexAttribDivisor(7, 1), glf);  // tscale

    // 解绑
    GLCALL(glf->glBindVertexArray(0), glf);
    GLCALL(glf->glBindBuffer(GL_ARRAY_BUFFER, 0), glf);
}

Renderer2D::~Renderer2D() {
    // 释放纹理池
    texturepool.reset();
}

void Renderer2D::add_texture_from_path(const std::string& path) {
    texturepool->rebuild_with_directory(path);
}

void Renderer2D::set_projection(const QMatrix4x4& projection) {
    shader_program->bind();
    shader_program->setUniformValue("projection", projection);
    shader_program->release();
}
// 更新需要更新的资源等等
void Renderer2D::update() { texturepool->processUploadQueue(); }

// 扩充矩形实例缓冲区
void Renderer2D::expandQuadDataBuffer() {
    bool need_update{false};
    while (max_quadcount < command_list.size()) {
        max_quadcount *= 2;
        need_update = true;
    }
    if (need_update) {
        // 绑定现有的VBO
        GLCALL(glf->glBindBuffer(GL_ARRAY_BUFFER, instance_dataBO), glf);
        // 直接用 glBufferData 重新分配，驱动会处理好旧内存的释放
        GLCALL(
            glf->glBufferData(GL_ARRAY_BUFFER, max_quadcount * sizeof(QuadData),
                              nullptr, GL_DYNAMIC_DRAW),
            glf);
        // 解绑
        GLCALL(glf->glBindBuffer(GL_ARRAY_BUFFER, 0), glf);
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
    GLCALL(glf->glVertexAttribPointer(
               0, 2, GL_FLOAT, false, sizeof(QuadData),
               (void*)(base_offset + offsetof(QuadData, pos))),
           glf);

    // 3~4 vec2 size
    GLCALL(glf->glVertexAttribPointer(
               1, 2, GL_FLOAT, false, sizeof(QuadData),
               (void*)(base_offset + offsetof(QuadData, size))),
           glf);

    // 5 f32 rotation
    GLCALL(glf->glVertexAttribPointer(
               2, 1, GL_FLOAT, false, sizeof(QuadData),
               (void*)(base_offset + offsetof(QuadData, rotation))),
           glf);

    // 6~9 vec4 color
    GLCALL(glf->glVertexAttribPointer(
               3, 4, GL_FLOAT, false, sizeof(QuadData),
               (void*)(base_offset + offsetof(QuadData, color))),
           glf);

    // 10~11 vec2 uv_scale
    GLCALL(glf->glVertexAttribPointer(
               4, 2, GL_FLOAT, false, sizeof(QuadData),
               (void*)(base_offset + offsetof(QuadData, uv_scale))),
           glf);

    // 10 uint layer_idx
    GLCALL(glf->glVertexAttribIPointer(
               5, 1, GL_UNSIGNED_INT, sizeof(QuadData),
               (void*)(base_offset + offsetof(QuadData, layer_idx))),
           glf);

    // 11 uint texalignmode
    GLCALL(glf->glVertexAttribIPointer(
               6, 1, GL_UNSIGNED_INT, sizeof(QuadData),
               (void*)(base_offset + offsetof(QuadData, talign))),
           glf);

    // 12 uint texscalemode
    GLCALL(glf->glVertexAttribIPointer(
               7, 1, GL_UNSIGNED_INT, sizeof(QuadData),
               (void*)(base_offset + offsetof(QuadData, tscale))),
           glf);
}

// 渲染
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
    command_batch.emplace_back(command_list.front().texture.gl_texture_array_id,
                               0, 1);

    // 从第二个指令开始遍历
    for (size_t i = 1; i < command_list.size();
         ++i) {  // [修正] 遍历 command_list
        // 检查当前指令是否可以合并到最后一个批次中
        if (command_list[i].texture.gl_texture_array_id ==
            command_batch.back().texture_array_id) {
            command_batch.back().instanceCount++;
        } else {
            // 不可合并，创建一个新的批次
            command_batch.emplace_back(
                command_list[i].texture.gl_texture_array_id, i, 1);
        }
    }

    // 清理原始的命令队列
    command_list.clear();
}

void Renderer2D::render() {
    if (quad_datas.empty() || command_batch.empty()) {
        quad_datas.clear();
        return;
    }

    // 绑定核心对象
    shader_program->bind();
    GLCALL(glf->glBindVertexArray(instance_dataAO), glf);
    GLCALL(glf->glBindBuffer(GL_ARRAY_BUFFER, instance_dataBO), glf);

    // 一次性上传所有实例数据
    GLCALL(
        glf->glBufferData(GL_ARRAY_BUFFER, quad_datas.size() * sizeof(QuadData),
                          quad_datas.data(), GL_DYNAMIC_DRAW),
        glf);

    // 循环遍历批处理，分批绘制
    for (const auto& batch : command_batch) {
        // 绑定这个批次需要的纹理
        // 激活纹理单元0
        GLCALL(glf->glActiveTexture(GL_TEXTURE0), glf);
        GLCALL(glf->glBindTexture(GL_TEXTURE_2D_ARRAY, batch.texture_array_id),
               glf);
        // 着色器采样器 u_samplerarray 使用纹理单元 0
        shader_program->setUniformValue("u_samplerarray", 0);

        // 更新顶点属性指针以指向当前批次的开头
        update_attribptrFromInstance(batch.startIndex);

        // 发起绘制调用
        GLCALL(
            glf->glDrawArraysInstanced(GL_TRIANGLES, 0, 6, batch.instanceCount),
            glf);
    }

    // 清理
    GLCALL(glf->glBindVertexArray(0), glf);
    GLCALL(glf->glBindBuffer(GL_ARRAY_BUFFER, 0), glf);
    shader_program->release();

    // 为下一帧做准备
    quad_datas.clear();
    command_batch.clear();
}
