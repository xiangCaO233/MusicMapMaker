#include <QFile>
#include <canvas/GLCanvas.hpp>
#include <render/Renderer2D.hpp>
#include <util/glcheck.hpp>

// 初始化着色器
void Renderer2D::initQuadShader() {
    // 初始化着色器
    initShader(quad_shader_program, "Quad",
               ":/glsl/canvas/quad_vshader.glsl.vert",
               ":/glsl/canvas/quad_fshader.glsl.frag");
    initShaderUBO(quad_shader_program, "Quad", "MaskStackUBO", 0);
}

void Renderer2D::initQuadObjectBuffers() {
    // quadbuffer
    // 初始化VAO
    GLCALL_V(cvs->glGenVertexArrays(1, &quad_instance_dataAO), cvs);
    // 绑定VAO
    GLCALL_V(cvs->glBindVertexArray(quad_instance_dataAO), cvs);

    // 初始化实例缓冲区
    GLCALL_V(cvs->glGenBuffers(1, &quad_instance_dataBO), cvs);
    // 绑定VBO
    GLCALL_V(cvs->glBindBuffer(GL_ARRAY_BUFFER, quad_instance_dataBO), cvs);
    GLCALL_V(cvs->glBufferData(GL_ARRAY_BUFFER,
                               max_quadcount * sizeof(PrimitiveData), nullptr,
                               GL_DYNAMIC_DRAW),
             cvs);

    // 0~2 vec2 pos
    GLCALL_V(cvs->glEnableVertexAttribArray(0), cvs);

    // 3~4 vec2 size
    GLCALL_V(cvs->glEnableVertexAttribArray(1), cvs);

    // 5 f32 rotation
    GLCALL_V(cvs->glEnableVertexAttribArray(2), cvs);

    // 6~9 vec4 color
    GLCALL_V(cvs->glEnableVertexAttribArray(3), cvs);

    // 10~11 vec2 radius
    GLCALL_V(cvs->glEnableVertexAttribArray(4), cvs);

    // 12 f32 radius_effect_param
    GLCALL_V(cvs->glEnableVertexAttribArray(5), cvs);

    // 13 uint radius_effect
    GLCALL_V(cvs->glEnableVertexAttribArray(6), cvs);

    // 14~15 vec2 uv_scale
    GLCALL_V(cvs->glEnableVertexAttribArray(7), cvs);

    // 16~17 vec2 group_size
    GLCALL_V(cvs->glEnableVertexAttribArray(8), cvs);

    // 18 uint no_filter
    GLCALL_V(cvs->glEnableVertexAttribArray(9), cvs);

    // 19 int layer_idx
    GLCALL_V(cvs->glEnableVertexAttribArray(10), cvs);

    // 20 uint texalignmode
    GLCALL_V(cvs->glEnableVertexAttribArray(11), cvs);

    // 21 uint texscalemode
    GLCALL_V(cvs->glEnableVertexAttribArray(12), cvs);

    // 22 uint primitive
    GLCALL_V(cvs->glEnableVertexAttribArray(13), cvs);

    updateQuadAttribptrFromInstance(0);

    GLCALL_V(cvs->glVertexAttribDivisor(0, 1), cvs);   // pos
    GLCALL_V(cvs->glVertexAttribDivisor(1, 1), cvs);   // size
    GLCALL_V(cvs->glVertexAttribDivisor(2, 1), cvs);   // rotation
    GLCALL_V(cvs->glVertexAttribDivisor(3, 1), cvs);   // color
    GLCALL_V(cvs->glVertexAttribDivisor(4, 1), cvs);   // radius
    GLCALL_V(cvs->glVertexAttribDivisor(5, 1), cvs);   // radius_effect_param
    GLCALL_V(cvs->glVertexAttribDivisor(6, 1), cvs);   // radius_effect
    GLCALL_V(cvs->glVertexAttribDivisor(7, 1), cvs);   // uv_scale
    GLCALL_V(cvs->glVertexAttribDivisor(8, 1), cvs);   // group_size
    GLCALL_V(cvs->glVertexAttribDivisor(9, 1), cvs);   // layer_idx
    GLCALL_V(cvs->glVertexAttribDivisor(10, 1), cvs);  // no_filter
    GLCALL_V(cvs->glVertexAttribDivisor(11, 1), cvs);  // talign
    GLCALL_V(cvs->glVertexAttribDivisor(12, 1), cvs);  // tscale
    GLCALL_V(cvs->glVertexAttribDivisor(13, 1), cvs);  // primitive

    // 解绑
    GLCALL_V(cvs->glBindVertexArray(0), cvs);
    GLCALL_V(cvs->glBindBuffer(GL_ARRAY_BUFFER, 0), cvs);
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
        GLCALL_V(cvs->glBindBuffer(GL_ARRAY_BUFFER, quad_instance_dataBO), cvs);
        // 直接用 glBufferData 重新分配，驱动会处理好旧内存的释放
        GLCALL_V(cvs->glBufferData(GL_ARRAY_BUFFER,
                                   max_quadcount * sizeof(PrimitiveData),
                                   nullptr, GL_DYNAMIC_DRAW),
                 cvs);
        // 解绑
        GLCALL_V(cvs->glBindBuffer(GL_ARRAY_BUFFER, 0), cvs);
    }
}

// 从指定矩形实例位置开始更新顶点数组指针
void Renderer2D::updateQuadAttribptrFromInstance(size_t instance_index) const {
    // 绑定矩形实例VBO
    GLCALL_V(cvs->glBindBuffer(GL_ARRAY_BUFFER, quad_instance_dataBO), cvs);
    // 计算当前实例索引在VBO中的字节偏移量
    size_t base_offset = instance_index * sizeof(PrimitiveData);

    // 0~2 vec2 pos
    GLCALL_V(cvs->glVertexAttribPointer(
                 0, 2, GL_FLOAT, false, sizeof(PrimitiveData),
                 (void*)(base_offset + offsetof(PrimitiveData, pos))),
             cvs);

    // 3~4 vec2 size
    GLCALL_V(cvs->glVertexAttribPointer(
                 1, 2, GL_FLOAT, false, sizeof(PrimitiveData),
                 (void*)(base_offset + offsetof(PrimitiveData, size))),
             cvs);

    // 5 f32 rotation
    GLCALL_V(cvs->glVertexAttribPointer(
                 2, 1, GL_FLOAT, false, sizeof(PrimitiveData),
                 (void*)(base_offset + offsetof(PrimitiveData, rotation))),
             cvs);

    // 6~9 vec4 color
    GLCALL_V(cvs->glVertexAttribPointer(
                 3, 4, GL_FLOAT, false, sizeof(PrimitiveData),
                 (void*)(base_offset + offsetof(PrimitiveData, color))),
             cvs);

    // 10~11 vec2 radius
    GLCALL_V(cvs->glVertexAttribPointer(
                 4, 2, GL_FLOAT, false, sizeof(PrimitiveData),
                 (void*)(base_offset + offsetof(PrimitiveData, radius))),
             cvs);

    // 12 f32 radius_effect_param
    GLCALL_V(cvs->glVertexAttribPointer(
                 5, 1, GL_FLOAT, false, sizeof(PrimitiveData),
                 (void*)(base_offset +
                         offsetof(PrimitiveData, radius_effect_param))),
             cvs);

    // 13 uint radius_effect
    GLCALL_V(cvs->glVertexAttribIPointer(
                 6, 1, GL_UNSIGNED_INT, sizeof(PrimitiveData),
                 (void*)(base_offset + offsetof(PrimitiveData, radius_effect))),
             cvs);

    // 14~15 vec2 uv_scale
    GLCALL_V(cvs->glVertexAttribPointer(
                 7, 2, GL_FLOAT, false, sizeof(PrimitiveData),
                 (void*)(base_offset + offsetof(PrimitiveData, uv_scale))),
             cvs);

    // 16~17 vec2 group_size
    GLCALL_V(cvs->glVertexAttribPointer(
                 8, 2, GL_FLOAT, false, sizeof(PrimitiveData),
                 (void*)(base_offset + offsetof(PrimitiveData, group_size))),
             cvs);

    // 18 int layer_idx
    GLCALL_V(cvs->glVertexAttribIPointer(
                 9, 1, GL_INT, sizeof(PrimitiveData),
                 (void*)(base_offset + offsetof(PrimitiveData, layer_idx))),
             cvs);

    // 19 uint no_filter
    GLCALL_V(cvs->glVertexAttribIPointer(
                 10, 1, GL_UNSIGNED_INT, sizeof(PrimitiveData),
                 (void*)(base_offset + offsetof(PrimitiveData, no_filter))),
             cvs);

    // 20 uint texalignmode
    GLCALL_V(cvs->glVertexAttribIPointer(
                 11, 1, GL_UNSIGNED_INT, sizeof(PrimitiveData),
                 (void*)(base_offset + offsetof(PrimitiveData, talign))),
             cvs);

    // 21 uint texscalemode
    GLCALL_V(cvs->glVertexAttribIPointer(
                 12, 1, GL_UNSIGNED_INT, sizeof(PrimitiveData),
                 (void*)(base_offset + offsetof(PrimitiveData, tscale))),
             cvs);

    // 22 uint primitive
    GLCALL_V(cvs->glVertexAttribIPointer(
                 13, 1, GL_UNSIGNED_INT, sizeof(PrimitiveData),
                 (void*)(base_offset + offsetof(PrimitiveData, primitive))),
             cvs);
}
