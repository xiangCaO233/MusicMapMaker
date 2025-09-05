#include <QFile>
#include <canvas/GLCanvas.hpp>
#include <render/Renderer2D.hpp>
#include <util/glcheck.hpp>

void Renderer2D::initCurveShader() {
    // 初始化着色器
    // 初始化着色器
    initShader(curve_shader_program, "Curve",
               ":/glsl/canvas/curve_vertex_shader.glsl.vert",
               ":/glsl/canvas/curve_fragment_shader.glsl.frag",
               ":/glsl/canvas/curve_geometry_shader.glsl.geom");
    initShaderUBO(primitive_shader_program, "Curve", "MaskStackUBO", 0);
}

void Renderer2D::initCurveObjectBuffers() {}

// 扩充曲线缓冲区
void Renderer2D::expandCurveDataBuffer() {}
