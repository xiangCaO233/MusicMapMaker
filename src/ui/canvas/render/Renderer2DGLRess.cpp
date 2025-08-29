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
        primitive_shader_program->bind();
        primitive_shader_program->setUniformValue("projection", projection);
        primitive_shader_program->release();
        // curve_shader_program->bind();
        // curve_shader_program->setUniformValue("projection", projection);
        // curve_shader_program->release();
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

        mesh_shader_program->bind();
        mesh_shader_program->setUniformValue("u_ActiveMaskLayerCount",
                                             int(mask_stack_cpu.size()));
        mesh_shader_program->release();

        primitive_shader_program->bind();
        primitive_shader_program->setUniformValue("u_ActiveMaskLayerCount",
                                                  int(mask_stack_cpu.size()));
        primitive_shader_program->release();

        // curve_shader_program->bind();
        // curve_shader_program->setUniformValue("u_ActiveMaskLayerCount",
        //                                       int(mask_stack_cpu.size()));
        // curve_shader_program->release();
        update_ubo = false;
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
        case PRIMITIVE: {
            return primitive_shader_program;
        }
        case CURVE: {
            return curve_shader_program;
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
        case PRIMITIVE: {
            return primitive_dataAO;
        }
        case CURVE: {
            return curve_dataAO;
        }
        default:
            return 0;
    }
}

// 批绘制
void Renderer2D::drawBatch(const RenderBatch& batch,
                           QOpenGLShaderProgram* shader, bool wireframe) const {
    switch (batch.type) {
        using enum CommandType;
        case QUAD: {
            // 更新矩形实例数组指针
            updateQuadAttribptrFromInstance(batch.startIndex);
            shader->setUniformValue("u_IsDrawingWireframe", false);
            DRAWCALL(cvs->glDrawArraysInstanced(GL_TRIANGLES, 0, 6,
                                                batch.elementCount),
                     cvs);
            if (wireframe) {
                shader->setUniformValue("u_IsDrawingWireframe", true);
                DRAWCALL(cvs->glDrawArraysInstanced(GL_LINE_LOOP, 0, 6,
                                                    batch.elementCount),
                         cvs);
            }
            break;
        }
        case PRIMITIVE: {
            shader->setUniformValue("u_IsDrawingWireframe", false);
            DRAWCALL(cvs->glDrawArrays(GL_POINTS, batch.startIndex,
                                       batch.elementCount),
                     cvs);
            if (wireframe) {
                shader->setUniformValue("u_IsDrawingWireframe", true);
                DRAWCALL(cvs->glDrawArrays(GL_LINE_LOOP, batch.startIndex,
                                           batch.elementCount),
                         cvs);
            }
            break;
        }
        case MESH:
        case CURVE: {
            shader->setUniformValue("u_IsDrawingWireframe", false);
            DRAWCALL(cvs->glDrawArrays(GL_TRIANGLES, batch.startIndex,
                                       batch.elementCount),
                     cvs);
            if (wireframe) {
                shader->setUniformValue("u_IsDrawingWireframe", true);
                DRAWCALL(cvs->glDrawArrays(GL_LINE_LOOP, batch.startIndex,
                                           batch.elementCount),
                         cvs);
            }
            break;
        }
    }
}

// 更新gpu数据
void Renderer2D::update_gpudata() {
    // === 1. 一次性上传所有GPU数据 ===
    // 矩形数据
    if (!quad_datas.empty()) {
        // 绑定矩形实例缓冲区
        GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, quad_instance_dataBO), cvs);
        // 一次性上传所有矩形实例数据
        GLCALL(cvs->glBufferData(GL_ARRAY_BUFFER,
                                 quad_datas.size() * sizeof(PrimitiveData),
                                 quad_datas.data(), GL_DYNAMIC_DRAW),
               cvs);
    }

    // 网格数据
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

    // 图元数据
    if (!primitive_datas.empty()) {
        // 绑定图元实例缓冲区
        GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, primitive_dataBO), cvs);
        // 一次性上传所有矩形实例数据
        GLCALL(cvs->glBufferData(GL_ARRAY_BUFFER,
                                 primitive_datas.size() * sizeof(PrimitiveData),
                                 primitive_datas.data(), GL_DYNAMIC_DRAW),
               cvs);
    }

    // 曲线数据
    if (!curve_datas.empty()) {
    }
}

// 渲染
void Renderer2D::render() {
    update();

    if (command_batchs.empty()) {
        quad_datas.clear();
        mesh_datas.clear();
        primitive_datas.clear();
        curve_datas.clear();
        return;
    }

    // 更新gpu数据
    update_gpudata();

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
            // if (!current_shader) return;
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
        drawBatch(batch, current_shader, draw_wireframe);
    }

    // === 3. 最终清理 ===
    GLCALL(cvs->glBindVertexArray(0), cvs);
    if (current_shader) {
        current_shader->release();
    }

    // 为下一帧做准备
    quad_datas.clear();
    mesh_datas.clear();
    primitive_datas.clear();
    curve_datas.clear();
    current_mesh_vertex_count = 0;
    current_curve_vertex_count = 0;
    command_batchs.clear();
}
