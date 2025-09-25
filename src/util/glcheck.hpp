#ifndef MMM_GLCHECK_HPP
#define MMM_GLCHECK_HPP

#include <QOpenGLFunctions_4_1_Core>
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
// 新宏 1: 用于调用返回 void 的 OpenGL 函数
#define GLCALL_V(function_call, gl_functions_ptr)        \
    glCallImpl(                                          \
        [&]() {                                          \
            mstat::gl_calls++;                           \
            (function_call); /* 直接执行，没有 return */ \
        },                                               \
        #function_call, gl_functions_ptr)

// 新宏 2: 用于调用有返回值的 OpenGL 函数
#define GLCALL_R(function_call, gl_functions_ptr)        \
    glCallImpl(                                          \
        [&]() {                                          \
            mstat::gl_calls++;                           \
            return (function_call); /* 执行并返回结果 */ \
        },                                               \
        #function_call, gl_functions_ptr)

// DRAWCALL 也做类似修改
#define DRAWCALL_V(function_call, gl_functions_ptr) \
    glCallImpl(                                     \
        [&]() {                                     \
            mstat::draw_calls++;                    \
            mstat::gl_calls++;                      \
            (function_call);                        \
        },                                          \
        #function_call, gl_functions_ptr)

#define DRAWCALL_R(func, f)      \
    glCallImpl(                  \
        [&]() {                  \
            mstat::draw_calls++; \
            mstat::gl_calls++;   \
            return func;         \
        },                       \
        #func, f)

#endif  // !MMM_GLCHECK_HPP
