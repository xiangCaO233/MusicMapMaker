#include <QOpenGLFunctions_4_1_Core>
#include <canvas/render/Renderer2D.hpp>
#include <type_traits>
#include <utility>

// C++17 的 if constexpr 的模板帮助函数
template <typename Func>
auto glCallImpl(Func func, const char* funcStr) {
    // 对 lambda 本身的返回类型进行判断
    if constexpr (std::is_void_v<decltype(func())>) {
        // lambda 返回 void
        // 调用 lambda
        func();
        if (GLenum error = glGetError() != GL_NO_ERROR) {
            qDebug() << "OpenGL Error in [" << funcStr << "]: " << error;
        }
        // 此分支无返回
    } else {
        // lambda 有返回值
        // 调用 lambda 并捕获结果
        auto&& result = func();
        if (GLenum error = glGetError() != GL_NO_ERROR) {
            qDebug() << "OpenGL Error in [" << funcStr << "]: " << error;
        }
        // 返回结果
        return std::forward<decltype(result)>(result);
    }
}

// 用于包装 OpenGL 调用并检查错误
#define GLCALL(func) glCallImpl([&]() { return func; }, #func)

void Renderer2D::init() {}

void Renderer2D::release() {}
