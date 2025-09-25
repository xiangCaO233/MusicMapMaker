#include <QFile>
#include <canvas/GLCanvas.hpp>
#include <render/Renderer2D.hpp>
#include <util/glcheck.hpp>

void Renderer2D::initMeshShader() {
    // 初始化着色器
    initShader(mesh_shader_program, "Mesh",
               ":/glsl/canvas/custom_meshvshader.glsl.vert",
               ":/glsl/canvas/custom_meshfshader.glsl.frag");
    initShaderUBO(mesh_shader_program, "Mesh", "MaskStackUBO", 0);
}

void Renderer2D::initMeshObjectBuffers() {
    // meshbuffer
    // 初始化VAO
    GLCALL_V(cvs->glGenVertexArrays(1, &mesh_dataAO), cvs);
    // 绑定VAO
    GLCALL_V(cvs->glBindVertexArray(mesh_dataAO), cvs);

    // 初始化网格顶点缓冲区
    GLCALL_V(cvs->glGenBuffers(1, &mesh_dataBO), cvs);
    // 绑定VBO
    GLCALL_V(cvs->glBindBuffer(GL_ARRAY_BUFFER, mesh_dataBO), cvs);
    GLCALL_V(cvs->glBufferData(GL_ARRAY_BUFFER,
                               max_mesh_vertexcount * sizeof(CustomVertex),
                               nullptr, GL_DYNAMIC_DRAW),
             cvs);

    // // 顶点位置
    // layout(location = 0) in vec2 vPosition;
    // 0~2 vec2 pos
    GLCALL_V(cvs->glEnableVertexAttribArray(0), cvs);
    GLCALL_V(
        cvs->glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(CustomVertex),
                                   (void*)(offsetof(CustomVertex, pos))),
        cvs);

    // // 顶点uv
    // layout(location = 1) in vec2 vTexCoord;
    GLCALL_V(cvs->glEnableVertexAttribArray(1), cvs);
    GLCALL_V(
        cvs->glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(CustomVertex),
                                   (void*)(offsetof(CustomVertex, uv))),
        cvs);

    // // 顶点颜色
    // layout(location = 2) in vec4 vColor;
    GLCALL_V(cvs->glEnableVertexAttribArray(2), cvs);
    GLCALL_V(
        cvs->glVertexAttribPointer(2, 4, GL_FLOAT, false, sizeof(CustomVertex),
                                   (void*)(offsetof(CustomVertex, color))),
        cvs);

    // // 顶点纹理信息
    // // 顶点使用的纹理在层中的尺寸比例
    // layout(location = 3) in vec2 aUVScale;
    GLCALL_V(cvs->glEnableVertexAttribArray(3), cvs);
    GLCALL_V(
        cvs->glVertexAttribPointer(3, 2, GL_FLOAT, false, sizeof(CustomVertex),
                                   (void*)(offsetof(CustomVertex, uv_scale))),
        cvs);

    // // 顶点使用的纹理的textureArray的统一尺寸
    // layout(location = 4) in vec2 aGroupSize;
    GLCALL_V(cvs->glEnableVertexAttribArray(4), cvs);
    GLCALL_V(
        cvs->glVertexAttribPointer(4, 2, GL_FLOAT, false, sizeof(CustomVertex),
                                   (void*)(offsetof(CustomVertex, group_size))),
        cvs);

    // // 顶点使用的纹理在层中层索引
    // layout(location = 5) in int aTextureLayerIdx;
    GLCALL_V(cvs->glEnableVertexAttribArray(5), cvs);
    GLCALL_V(
        cvs->glVertexAttribIPointer(5, 1, GL_INT, sizeof(CustomVertex),
                                    (void*)(offsetof(CustomVertex, layer_idx))),
        cvs);

    // // 顶点使用的纹理是否应用ubo蒙版效果
    // layout(location = 6) in uint aNoFilter;
    GLCALL_V(cvs->glEnableVertexAttribArray(6), cvs);
    GLCALL_V(
        cvs->glVertexAttribIPointer(6, 1, GL_UNSIGNED_INT, sizeof(CustomVertex),
                                    (void*)(offsetof(CustomVertex, no_filter))),
        cvs);

    // 在点集合中的位置(0内部,1头部,-1尾部)
    // layout(location = 7) in int aGroupPos;
    GLCALL_V(cvs->glEnableVertexAttribArray(7), cvs);
    GLCALL_V(
        cvs->glVertexAttribIPointer(7, 1, GL_INT, sizeof(CustomVertex),
                                    (void*)(offsetof(CustomVertex, group_pos))),
        cvs);

    // 解绑
    GLCALL_V(cvs->glBindVertexArray(0), cvs);
    GLCALL_V(cvs->glBindBuffer(GL_ARRAY_BUFFER, 0), cvs);
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
        GLCALL_V(cvs->glBindBuffer(GL_ARRAY_BUFFER, mesh_dataBO), cvs);
        GLCALL_V(cvs->glBufferData(GL_ARRAY_BUFFER,
                                   max_mesh_vertexcount * sizeof(CustomVertex),
                                   nullptr, GL_DYNAMIC_DRAW),
                 cvs);
        // 解绑
        GLCALL_V(cvs->glBindBuffer(GL_ARRAY_BUFFER, 0), cvs);
    }
}
