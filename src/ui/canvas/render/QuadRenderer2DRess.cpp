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

    // 初始化实例缓冲区
    GLCALL(cvs->glGenBuffers(1, &quad_instance_dataBO), cvs);
    // 绑定VBO
    GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, quad_instance_dataBO), cvs);
    GLCALL(cvs->glBufferData(GL_ARRAY_BUFFER,
                             max_quadcount * sizeof(PrimitiveData), nullptr,
                             GL_DYNAMIC_DRAW),
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
        GLCALL(cvs->glBufferData(GL_ARRAY_BUFFER,
                                 max_quadcount * sizeof(PrimitiveData), nullptr,
                                 GL_DYNAMIC_DRAW),
               cvs);
        // 解绑
        GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, 0), cvs);
    }
}

// 从指定矩形实例位置开始更新顶点数组指针
void Renderer2D::updateQuadAttribptrFromInstance(size_t instance_index) const {
    // 绑定矩形实例VBO
    GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, quad_instance_dataBO), cvs);
    // 计算当前实例索引在VBO中的字节偏移量
    size_t base_offset = instance_index * sizeof(PrimitiveData);

    // 0~2 vec2 pos
    GLCALL(cvs->glVertexAttribPointer(
               0, 2, GL_FLOAT, false, sizeof(PrimitiveData),
               (void*)(base_offset + offsetof(PrimitiveData, pos))),
           cvs);

    // 3~4 vec2 size
    GLCALL(cvs->glVertexAttribPointer(
               1, 2, GL_FLOAT, false, sizeof(PrimitiveData),
               (void*)(base_offset + offsetof(PrimitiveData, size))),
           cvs);

    // 5 f32 rotation
    GLCALL(cvs->glVertexAttribPointer(
               2, 1, GL_FLOAT, false, sizeof(PrimitiveData),
               (void*)(base_offset + offsetof(PrimitiveData, rotation))),
           cvs);

    // 6~9 vec4 color
    GLCALL(cvs->glVertexAttribPointer(
               3, 4, GL_FLOAT, false, sizeof(PrimitiveData),
               (void*)(base_offset + offsetof(PrimitiveData, color))),
           cvs);

    // 10~11 vec2 radius
    GLCALL(cvs->glVertexAttribPointer(
               4, 2, GL_FLOAT, false, sizeof(PrimitiveData),
               (void*)(base_offset + offsetof(PrimitiveData, radius))),
           cvs);

    // 12 f32 radius_effect_param
    GLCALL(cvs->glVertexAttribPointer(
               5, 1, GL_FLOAT, false, sizeof(PrimitiveData),
               (void*)(base_offset +
                       offsetof(PrimitiveData, radius_effect_param))),
           cvs);

    // 13 uint radius_effect
    GLCALL(cvs->glVertexAttribIPointer(
               6, 1, GL_UNSIGNED_INT, sizeof(PrimitiveData),
               (void*)(base_offset + offsetof(PrimitiveData, radius_effect))),
           cvs);

    // 14~15 vec2 uv_scale
    GLCALL(cvs->glVertexAttribPointer(
               7, 2, GL_FLOAT, false, sizeof(PrimitiveData),
               (void*)(base_offset + offsetof(PrimitiveData, uv_scale))),
           cvs);

    // 16~17 vec2 group_size
    GLCALL(cvs->glVertexAttribPointer(
               8, 2, GL_FLOAT, false, sizeof(PrimitiveData),
               (void*)(base_offset + offsetof(PrimitiveData, group_size))),
           cvs);

    // 18 int layer_idx
    GLCALL(cvs->glVertexAttribIPointer(
               9, 1, GL_INT, sizeof(PrimitiveData),
               (void*)(base_offset + offsetof(PrimitiveData, layer_idx))),
           cvs);

    // 19 uint no_filter
    GLCALL(cvs->glVertexAttribIPointer(
               10, 1, GL_UNSIGNED_INT, sizeof(PrimitiveData),
               (void*)(base_offset + offsetof(PrimitiveData, no_filter))),
           cvs);

    // 20 uint texalignmode
    GLCALL(cvs->glVertexAttribIPointer(
               11, 1, GL_UNSIGNED_INT, sizeof(PrimitiveData),
               (void*)(base_offset + offsetof(PrimitiveData, talign))),
           cvs);

    // 21 uint texscalemode
    GLCALL(cvs->glVertexAttribIPointer(
               12, 1, GL_UNSIGNED_INT, sizeof(PrimitiveData),
               (void*)(base_offset + offsetof(PrimitiveData, tscale))),
           cvs);
}
