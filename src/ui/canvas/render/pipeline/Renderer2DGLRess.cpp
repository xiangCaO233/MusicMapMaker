#include <QFile>
#include <canvas/GLCanvas.hpp>
#include <render/Renderer2D.hpp>
#include <util/glcheck.hpp>

void Renderer2D::initShader(QOpenGLShaderProgram*& shader,
                            const char* debug_name,
                            const char* vertex_shader_source_path,
                            const char* fragment_shader_source_path,
                            const char* geometry_shader_source_path) const {
    // 初始化着色器
    shader = new QOpenGLShaderProgram();
    // 从资源qrc加载
    QFile vert_source(vertex_shader_source_path);
    QFile frag_source(fragment_shader_source_path);

    // 检查文件是否成功打开
    if (!vert_source.open(QIODevice::ReadOnly | QIODevice::Text)) {
        auto errormsg = vert_source.errorString();
        auto errorstr = errormsg.toStdString();
        qDebug() << "Failed to open " << debug_name
                 << "vertex source file:" << errorstr;
    }
    if (!frag_source.open(QIODevice::ReadOnly | QIODevice::Text)) {
        auto errormsg = frag_source.errorString();
        auto errorstr = errormsg.toStdString();
        qDebug() << "Failed to open" << debug_name
                 << "frag source file:" << errorstr;
    }
    // 用QTextStream读取内容
    QTextStream vertin(&vert_source);
    QTextStream fragin(&frag_source);

    auto vertex_shader_qstr = vertin.readAll();
    auto fragment_shader_qstr = fragin.readAll();

    // 关闭文件
    vert_source.close();
    frag_source.close();

    // 编译链接着色器
    if (!shader->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                         vertex_shader_qstr)) {
        qCritical() << "Renderer " << debug_name
                    << "Vertex Shader compilation failed:" << shader->log();
    }
    if (!shader->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                         fragment_shader_qstr)) {
        qCritical() << "Renderer " << debug_name
                    << " Fragment Shader compilation failed:" << shader->log();
    }

    if (geometry_shader_source_path) {
        QFile geom_source(geometry_shader_source_path);
        if (!geom_source.open(QIODevice::ReadOnly | QIODevice::Text)) {
            auto errormsg = geom_source.errorString();
            auto errorstr = errormsg.toStdString();
            qDebug() << "Failed to open " << debug_name
                     << " geometry source file:" << errorstr;
        }
        QTextStream geomin(&geom_source);
        auto geometry_shader_qstr = geomin.readAll();
        geom_source.close();
        if (!shader->addShaderFromSourceCode(QOpenGLShader::Geometry,
                                             geometry_shader_qstr)) {
            qCritical() << "Renderer " << debug_name
                        << " Geometry Shader compilation failed:"
                        << shader->log();
        }
    }

    if (!shader->link()) {
        qCritical() << debug_name << " Shader link failed:" << shader->log();
    }
}
void Renderer2D::initShaderUBO(QOpenGLShaderProgram*& shader,
                               const char* debug_name,
                               const char* name_in_shader, uint32_t ubo_index) {
    // 检查是否找到了UBO块（如果拼写错误或被优化掉，可能找不到）
    if (GLuint mask_ubo_index = GLCALL(
            cvs->glGetUniformBlockIndex(shader->programId(), name_in_shader),
            cvs);
        mask_ubo_index != GL_INVALID_INDEX) {
        // 将 uniform block 索引，绑定到绑定点 0
        GLCALL(cvs->glUniformBlockBinding(shader->programId(), mask_ubo_index,
                                          ubo_index),
               cvs);
    } else {
        qWarning() << "Could not find uniform block '" << name_in_shader
                   << "' in " << debug_name << " shader program.";
    }
}

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

// 更新fbo
void Renderer2D::update_fbo() {
    // 高效地更新每个FBO的纹理尺寸，而不是销毁重建
    mainFBO->update_viewport(phisical_viewport);
    compositeFBO->update_viewport(phisical_viewport);

    glm::vec2 blur_size = {phisical_viewport.x / 4.0f,
                           phisical_viewport.y / 4.0f};
    gaussianBlurFBOA->update_viewport(blur_size);
    gaussianBlurFBOB->update_viewport(blur_size);
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
        curve_shader_program->bind();
        curve_shader_program->setUniformValue("projection", projection);
        curve_shader_program->release();
        update_fbo();
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

    // 绑定多目标FBO
    mainFBO->bind();

    // 激活MRT
    // 列出将要写入的所有颜色附件
    GLenum drawBuffers[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    // 告诉OpenGL，接下来的绘制操作，片元着色器的 location=0
    // 的输出去附件0，location=1 的输出去附件1
    // 2是附件的数量
    GLCALL(cvs->glDrawBuffers(2, drawBuffers), cvs);
    GLCALL(cvs->glClearColor(.23f, .23f, .23f, .23f), cvs);
    GLCALL(cvs->glClear(GL_COLOR_BUFFER_BIT), cvs);

    // === 2. 在单个循环中通过状态追踪进行渲染 ===
    QOpenGLShaderProgram* current_shader{nullptr};
    uint32_t current_vao{0};

    for (const auto& batch : command_batchs) {
        if (batch.type == CommandType::MESH) {
            continue;
        }
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
        GLCALL(cvs->glActiveTexture(GL_TEXTURE1), cvs);
        GLCALL(cvs->glBindTexture(GL_TEXTURE_2D, mainFBO->textures()[1]), cvs);
        current_shader->setUniformValue("u_samplerarray", 0);
        current_shader->setUniformValue("glowmask", 1);

        // --- 发起绘制调用 ---
        drawBatch(batch, current_shader, draw_wireframe);
    }

    // === 3. 最终清理 ===
    GLCALL(cvs->glBindVertexArray(0), cvs);
    if (current_shader) {
        current_shader->release();
    }

    mainFBO->release();

    // 后期处理
    afterEffect();

    // 混合着色
    composite();

    // 为下一帧做准备
    quad_datas.clear();
    mesh_datas.clear();
    primitive_datas.clear();
    curve_datas.clear();
    current_mesh_vertex_count = 0;
    current_curve_vertex_count = 0;
    command_batchs.clear();
}

// 后期处理
void Renderer2D::afterEffect() {
    gaussian_blur_shader->bind();
    GLCALL(cvs->glBindVertexArray(fullScreenAO), cvs);
    bool horizontal = true;
    bool first_iteration = true;

    gaussian_blur_shader->setUniformValue("gaussian_source", 2);

    for (int i = 0; i < blur_iteration_count; i++) {
        // 乒乓操作
        // --- Pass 1: 横向模糊 ---
        // 写入 B
        gaussianBlurFBOB->bind();
        GLCALL(cvs->glClear(GL_COLOR_BUFFER_BIT), cvs);

        // 绑定源纹理
        if (first_iteration) {
            // 第一次读取辉光遮罩
            GLCALL(cvs->glActiveTexture(GL_TEXTURE2), cvs);
            GLCALL(cvs->glBindTexture(GL_TEXTURE_2D, mainFBO->textures()[1]),
                   cvs);
            GLCALL(cvs->glBindSampler(2, 0), cvs);
            GLCALL(cvs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                        GL_LINEAR),
                   cvs);
            GLCALL(cvs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                                        GL_LINEAR),
                   cvs);
        } else {
            // 后续读取A的结果
            GLCALL(cvs->glActiveTexture(GL_TEXTURE2), cvs);
            GLCALL(cvs->glBindTexture(GL_TEXTURE_2D,
                                      gaussianBlurFBOA->textures()[0]),
                   cvs);
            GLCALL(cvs->glBindSampler(2, 0), cvs);
            GLCALL(cvs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                        GL_LINEAR),
                   cvs);
            GLCALL(cvs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                                        GL_LINEAR),
                   cvs);
        }
        gaussian_blur_shader->setUniformValue("horizontal", true);
        // 绘制全屏矩形
        GLCALL(cvs->glDrawArrays(GL_TRIANGLES, 0, 6), cvs);
        gaussianBlurFBOB->release();

        // --- Pass 2: 纵向模糊 ---
        // 写入 A
        gaussianBlurFBOA->bind();
        GLCALL(cvs->glClear(GL_COLOR_BUFFER_BIT), cvs);

        // 绑定源纹理 (现在是B)
        GLCALL(cvs->glActiveTexture(GL_TEXTURE2), cvs);
        GLCALL(
            cvs->glBindTexture(GL_TEXTURE_2D, gaussianBlurFBOA->textures()[0]),
            cvs);
        GLCALL(cvs->glBindSampler(2, 0), cvs);
        GLCALL(cvs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                    GL_LINEAR),
               cvs);
        GLCALL(cvs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                                    GL_LINEAR),
               cvs);
        gaussian_blur_shader->setUniformValue("horizontal", false);
        GLCALL(cvs->glDrawArrays(GL_TRIANGLES, 0, 6), cvs);
        gaussianBlurFBOA->release();

        first_iteration = false;
    }
    gaussian_blur_shader->release();
    GLCALL(cvs->glBindVertexArray(0), cvs);
}

// 混合着色
void Renderer2D::composite() {}
