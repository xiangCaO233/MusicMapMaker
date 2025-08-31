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
    GLCALL(cvs->glGenVertexArrays(1, &fullScreenAO), cvs);
}
