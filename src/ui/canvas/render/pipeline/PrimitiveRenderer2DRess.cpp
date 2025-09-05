#include <QFile>
#include <canvas/GLCanvas.hpp>
#include <render/Renderer2D.hpp>
#include <util/glcheck.hpp>

void Renderer2D::initPrimitiveShader() {
    // 初始化着色器
    initShader(primitive_shader_program, "Primitive",
               ":/glsl/canvas/primitive_vertex_shader.glsl.vert",
               ":/glsl/canvas/primitive_fragment_shader.glsl.frag",
               ":/glsl/canvas/primitive_geometry_shader.glsl.geom");
    initShaderUBO(primitive_shader_program, "Primitive", "MaskStackUBO", 0);
}

void Renderer2D::initPrimitiveObjectBuffers() {
    // primitivebuffer
    // 初始化VAO
    GLCALL(cvs->glGenVertexArrays(1, &primitive_dataAO), cvs);
    // 绑定VAO
    GLCALL(cvs->glBindVertexArray(primitive_dataAO), cvs);

    // 初始化实例缓冲区
    GLCALL(cvs->glGenBuffers(1, &primitive_dataBO), cvs);
    // 绑定VBO
    GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, primitive_dataBO), cvs);
    GLCALL(cvs->glBufferData(GL_ARRAY_BUFFER,
                             max_primitivecount * sizeof(PrimitiveData),
                             nullptr, GL_DYNAMIC_DRAW),
           cvs);

    // 0~2 vec2 pos
    GLCALL(cvs->glEnableVertexAttribArray(0), cvs);
    GLCALL(
        cvs->glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(PrimitiveData),
                                   (void*)(offsetof(PrimitiveData, pos))),
        cvs);

    // 3~4 vec2 size
    GLCALL(cvs->glEnableVertexAttribArray(1), cvs);
    GLCALL(
        cvs->glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(PrimitiveData),
                                   (void*)(offsetof(PrimitiveData, size))),
        cvs);

    // 5 f32 rotation
    GLCALL(cvs->glEnableVertexAttribArray(2), cvs);
    GLCALL(
        cvs->glVertexAttribPointer(2, 1, GL_FLOAT, false, sizeof(PrimitiveData),
                                   (void*)(offsetof(PrimitiveData, rotation))),
        cvs);

    // 6~9 vec4 color
    GLCALL(cvs->glEnableVertexAttribArray(3), cvs);
    GLCALL(
        cvs->glVertexAttribPointer(3, 4, GL_FLOAT, false, sizeof(PrimitiveData),
                                   (void*)(offsetof(PrimitiveData, color))),
        cvs);

    // 10~11 vec2 radius
    GLCALL(cvs->glEnableVertexAttribArray(4), cvs);
    GLCALL(
        cvs->glVertexAttribPointer(4, 2, GL_FLOAT, false, sizeof(PrimitiveData),
                                   (void*)(offsetof(PrimitiveData, radius))),
        cvs);

    // 12 f32 radius_effect_param
    GLCALL(cvs->glEnableVertexAttribArray(5), cvs);
    GLCALL(cvs->glVertexAttribPointer(
               5, 1, GL_FLOAT, false, sizeof(PrimitiveData),
               (void*)(offsetof(PrimitiveData, radius_effect_param))),
           cvs);

    // 13 uint radius_effect
    GLCALL(cvs->glEnableVertexAttribArray(6), cvs);
    GLCALL(cvs->glVertexAttribIPointer(
               6, 1, GL_UNSIGNED_INT, sizeof(PrimitiveData),
               (void*)(offsetof(PrimitiveData, radius_effect))),
           cvs);

    // 14~15 vec2 uv_scale
    GLCALL(cvs->glEnableVertexAttribArray(7), cvs);
    GLCALL(
        cvs->glVertexAttribPointer(7, 2, GL_FLOAT, false, sizeof(PrimitiveData),
                                   (void*)(offsetof(PrimitiveData, uv_scale))),
        cvs);

    // 16~17 vec2 group_size
    GLCALL(cvs->glEnableVertexAttribArray(8), cvs);
    GLCALL(cvs->glVertexAttribPointer(
               8, 2, GL_FLOAT, false, sizeof(PrimitiveData),
               (void*)(offsetof(PrimitiveData, group_size))),
           cvs);

    // 18 uint no_filter
    GLCALL(cvs->glEnableVertexAttribArray(9), cvs);
    GLCALL(cvs->glVertexAttribIPointer(
               9, 1, GL_INT, sizeof(PrimitiveData),
               (void*)(offsetof(PrimitiveData, layer_idx))),
           cvs);

    // 19 int layer_idx
    GLCALL(cvs->glEnableVertexAttribArray(10), cvs);
    GLCALL(cvs->glVertexAttribIPointer(
               10, 1, GL_UNSIGNED_INT, sizeof(PrimitiveData),
               (void*)(offsetof(PrimitiveData, no_filter))),
           cvs);

    // 20 uint texalignmode
    GLCALL(cvs->glEnableVertexAttribArray(11), cvs);
    GLCALL(cvs->glVertexAttribIPointer(
               11, 1, GL_UNSIGNED_INT, sizeof(PrimitiveData),
               (void*)(offsetof(PrimitiveData, talign))),
           cvs);

    // 21 uint texscalemode
    GLCALL(cvs->glEnableVertexAttribArray(12), cvs);
    GLCALL(cvs->glVertexAttribIPointer(
               12, 1, GL_UNSIGNED_INT, sizeof(PrimitiveData),
               (void*)(offsetof(PrimitiveData, tscale))),
           cvs);

    // 22 uint primitive
    GLCALL(cvs->glEnableVertexAttribArray(13), cvs);
    GLCALL(cvs->glVertexAttribIPointer(
               13, 1, GL_UNSIGNED_INT, sizeof(PrimitiveData),
               (void*)(offsetof(PrimitiveData, primitive))),
           cvs);
}

// 扩充图元缓冲区
void Renderer2D::expandPrimitiveDataBuffer() {
    bool need_update{false};

    while (max_primitivecount < primitive_command_list.size()) {
        max_primitivecount *= 2;
        need_update = true;
    }

    if (need_update) {
        // 绑定矩形实例VBO
        GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, primitive_dataBO), cvs);
        // 直接用 glBufferData 重新分配，驱动会处理好旧内存的释放
        GLCALL(cvs->glBufferData(GL_ARRAY_BUFFER,
                                 max_primitivecount * sizeof(PrimitiveData),
                                 nullptr, GL_DYNAMIC_DRAW),
               cvs);
        // 解绑
        GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, 0), cvs);
    }
}
