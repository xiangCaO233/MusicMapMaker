#include <QFile>
#include <canvas/GLCanvas.hpp>
#include <render/Renderer2D.hpp>
#include <type_traits>
#include <util/statistic.hpp>
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

void Renderer2D::initMeshBuffers() {
    // meshbuffer
    // 初始化VAO
    GLCALL(cvs->glGenVertexArrays(1, &mesh_dataAO), cvs);
    // 绑定VAO
    GLCALL(cvs->glBindVertexArray(mesh_dataAO), cvs);

    // 初始化网格顶点缓冲区
    GLCALL(cvs->glGenBuffers(1, &mesh_dataBO), cvs);
    // 绑定VBO
    GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, mesh_dataBO), cvs);
    GLCALL(cvs->glBufferData(GL_ARRAY_BUFFER,
                             max_mesh_vertexcount * sizeof(CustomVertex),
                             nullptr, GL_DYNAMIC_DRAW),
           cvs);

    // // 顶点位置
    // layout(location = 0) in vec2 vPosition;
    // 0~2 vec2 pos
    GLCALL(cvs->glEnableVertexAttribArray(0), cvs);
    GLCALL(
        cvs->glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(CustomVertex),
                                   (void*)(offsetof(CustomVertex, pos))),
        cvs);

    // // 顶点uv
    // layout(location = 1) in vec2 vTexCoord;
    GLCALL(cvs->glEnableVertexAttribArray(1), cvs);
    GLCALL(
        cvs->glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(CustomVertex),
                                   (void*)(offsetof(CustomVertex, uv))),
        cvs);

    // // 顶点颜色
    // layout(location = 2) in vec4 vColor;
    GLCALL(cvs->glEnableVertexAttribArray(2), cvs);
    GLCALL(
        cvs->glVertexAttribPointer(2, 4, GL_FLOAT, false, sizeof(CustomVertex),
                                   (void*)(offsetof(CustomVertex, color))),
        cvs);

    // // 顶点纹理信息
    // // 顶点使用的纹理在层中的尺寸比例
    // layout(location = 3) in vec2 aUVScale;
    GLCALL(cvs->glEnableVertexAttribArray(3), cvs);
    GLCALL(
        cvs->glVertexAttribPointer(3, 2, GL_FLOAT, false, sizeof(CustomVertex),
                                   (void*)(offsetof(CustomVertex, uv_scale))),
        cvs);

    // // 顶点使用的纹理的textureArray的统一尺寸
    // layout(location = 4) in vec2 aGroupSize;
    GLCALL(cvs->glEnableVertexAttribArray(4), cvs);
    GLCALL(
        cvs->glVertexAttribPointer(4, 2, GL_FLOAT, false, sizeof(CustomVertex),
                                   (void*)(offsetof(CustomVertex, group_size))),
        cvs);

    // // 顶点使用的纹理在层中层索引
    // layout(location = 5) in int aTextureLayerIdx;
    GLCALL(cvs->glEnableVertexAttribArray(5), cvs);
    GLCALL(
        cvs->glVertexAttribIPointer(5, 1, GL_INT, sizeof(CustomVertex),
                                    (void*)(offsetof(CustomVertex, layer_idx))),
        cvs);

    // // 顶点使用的纹理是否应用ubo蒙版效果
    // layout(location = 6) in uint aNoFilter;
    GLCALL(cvs->glEnableVertexAttribArray(6), cvs);
    GLCALL(
        cvs->glVertexAttribIPointer(6, 1, GL_UNSIGNED_INT, sizeof(CustomVertex),
                                    (void*)(offsetof(CustomVertex, no_filter))),
        cvs);

    // 解绑
    GLCALL(cvs->glBindVertexArray(0), cvs);
    GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, 0), cvs);
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
