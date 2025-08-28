#include <QFile>
#include <canvas/GLCanvas.hpp>
#include <render/Renderer2D.hpp>
#include <type_traits>
#include <util/statistic.hpp>
#include <utility>
#include <vector>

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
#define GLCALL(func, f)       \
    glCallImpl(               \
        [&]() {               \
            stat::gl_calls++; \
            return func;      \
        },                    \
        #func, f)
#define DRAWCALL(func, f)       \
    glCallImpl(                 \
        [&]() {                 \
            stat::draw_calls++; \
            stat::gl_calls++;   \
            return func;        \
        },                      \
        #func, f)

void Renderer2D::initMaskUBO() {
    // 初始化ubo
    GLCALL(cvs->glGenBuffers(1, &mask_uBO), cvs);
    // 绑定ubo
    GLCALL(cvs->glBindBuffer(GL_UNIFORM_BUFFER, mask_uBO), cvs);
    // 将UBO缓冲对象，也连接到绑定点 0
    // 这一步确保了绑定点0实际连接的是我们创建的 m_mask_ubo 这个GPU缓冲区
    GLCALL(cvs->glBindBufferBase(GL_UNIFORM_BUFFER, 0, mask_uBO), cvs);
    GLCALL(cvs->glBindBuffer(GL_UNIFORM_BUFFER, 0), cvs);
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
        mesh_shader_program->bind();
        mesh_shader_program->setUniformValue("projection", projection);
        mesh_shader_program->release();
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
        // 需要一个uniform告诉着色器当前有多少个活跃的蒙版层
        mesh_shader_program->bind();
        mesh_shader_program->setUniformValue("u_ActiveMaskLayerCount",
                                             int(mask_stack_cpu.size()));
        mesh_shader_program->release();
        update_ubo = false;
    }
}

// 批绘制
void Renderer2D::drawBatch(const RenderBatch& batch, GLenum mode) const {
    switch (batch.type) {
        using enum CommandType;
        case QUAD: {
            // 更新矩形实例数组指针
            updateQuadAttribptrFromInstance(batch.startIndex);
            // GLCALL(cvs->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA),
            // cvs); GLCALL(cvs->glDepthMask(GL_TRUE), cvs);
            DRAWCALL(cvs->glDrawArraysInstanced(mode, 0, 6, batch.elementCount),
                     cvs);
            break;
        }
        case MESH: {
            // GLCALL(cvs->glBlendFunc(GL_SRC_ALPHA, GL_ONE), cvs);
            // GLCALL(cvs->glDepthMask(GL_FALSE), cvs);
            DRAWCALL(
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
                                 quad_datas.size() * sizeof(PrimitiveData),
                                 quad_datas.data(), GL_DYNAMIC_DRAW),
               cvs);
    }
    if (!mesh_datas.empty()) {
        // 绑定网格缓冲区
        GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, mesh_dataBO), cvs);

        // 规范网格数据
        std::vector<CustomVertex> stagingVertexBuffer;
        stagingVertexBuffer.reserve(current_mesh_vertex_count);
        for (const auto& meshData : mesh_datas) {
            stagingVertexBuffer.insert(stagingVertexBuffer.end(),
                                       meshData.vertices.begin(),
                                       meshData.vertices.end());
        }

        // 一次性上传所有网格数据
        GLCALL(
            cvs->glBufferData(GL_ARRAY_BUFFER,
                              stagingVertexBuffer.size() * sizeof(CustomVertex),
                              stagingVertexBuffer.data(), GL_DYNAMIC_DRAW),
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
    current_mesh_vertex_count = 0;
    command_batchs.clear();
}
