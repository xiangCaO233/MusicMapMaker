#include <QFile>
#include <canvas/GLCanvas.hpp>
#include <render/Renderer2D.hpp>
#include <util/glcheck.hpp>

// 初始化后期着色器/gl资源
void Renderer2D::initAfterEffectShaders() {
    // 初始化着色器
    initShader(gaussian_blur_shader, "GaussianBlur",
               ":/glsl/canvas/aftereffect/fullscreen_vertex_shader.glsl.vert",
               ":/glsl/canvas/aftereffect/gaussian_fragment_shader.glsl.frag");
    initShader(composite_shader, "Composite",
               ":/glsl/canvas/aftereffect/fullscreen_vertex_shader.glsl.vert",
               ":/glsl/canvas/aftereffect/composite_fragment_shader.glsl.frag");
}

void Renderer2D::initAfterEffectObjectBuffers() {
    GLCALL_V(cvs->glGenVertexArrays(1, &fullScreenAO), cvs);
}
