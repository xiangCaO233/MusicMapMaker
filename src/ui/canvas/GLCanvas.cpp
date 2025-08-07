#include <qlogging.h>

#include <QGuiApplication>
#include <QOpenGLFunctions>
#include <QScreen>
#include <canvas/GLCanvas.hpp>
#include <canvas/render/Renderer2D.hpp>
#include <chrono>
#include <render/texture/TexturePool.hpp>
#include <type_traits>
#include <utility>

#include "render/MPainter.hpp"
#include "render/texture/TexMode.hpp"

// C++17 的 if constexpr 的模板帮助函数
template <typename Func>
auto glCallImpl(Func func, const char* funcStr) {
    // 1. 先清除所有历史错误，确保我们只捕获当前调用的错误
    while (glGetError() != GL_NO_ERROR);

    // 2. 对 lambda 本身的返回类型进行判断
    if constexpr (std::is_void_v<decltype(func())>) {
        func();  // 调用 lambda
    } else {
        auto&& result = func();  // 调用并捕获结果
        // 检查错误在调用之后
        if (GLenum error = glGetError(); error != GL_NO_ERROR) {
            qDebug() << "OpenGL Error in [" << funcStr << "]: " << error
                     << "(Hex: 0x" << Qt::hex << error << Qt::dec << ")";
        }
        return std::forward<decltype(result)>(result);
    }

    // 针对void返回类型的lambda，在调用后检查错误
    if (GLenum error = glGetError(); error != GL_NO_ERROR) {
        qDebug() << "OpenGL Error in [" << funcStr << "]: " << error
                 << "(Hex: 0x" << Qt::hex << error << Qt::dec << ")";
    }
}

// 用于包装 OpenGL 调用并检查错误
#define GLCALL(func) glCallImpl([&]() { return func; }, #func)

// 构造GLCanvas
GLCanvas::GLCanvas() {
    // 初始化帧率计数器
    fpsCounter = new FrameRateCounter();

    // 更新fps显示内容
    connect(fpsCounter, &FrameRateCounter::fpsUpdated, this,
            &GLCanvas::updateFpsDisplay);

    auto refreshRate = QGuiApplication::primaryScreen()->refreshRate();
    qDebug() << "显示器刷新率 : " << refreshRate;

    // 帧间隔
    auto des_update_time = 1000.0 / refreshRate;
    qDebug() << "目标帧间隔 : " << std::to_string(des_update_time);
}

// 析构GLCanvas
GLCanvas::~GLCanvas() { delete fpsCounter; }

void GLCanvas::updateFpsDisplay(int fps) {
    QString title_suffix =
        QString(
            "%1 FPS(frametime: "
            "%2 us | updatetime(qt): %3 ms)")
            .arg(fps)
            .arg(std::chrono::duration_cast<std::chrono::microseconds>(
                     pre_frame_time)
                     .count())
            .arg(actual_update_time);
    emit update_window_suffix(title_suffix);
}

void GLCanvas::initializeGL() {
    initializeOpenGLFunctions();
    // 查询opengl版本
    auto version = GLCALL(glGetString(GL_VERSION));
    qDebug() << "OpenGL 版本: "
             << std::string(reinterpret_cast<const char*>(version));

    // 查询最大支持抗锯齿MSAA倍率
    GLint maxSamples;
    GLCALL(glGetIntegerv(GL_MAX_SAMPLES, &maxSamples));

    // 初始化驱动信息
    qDebug() << "启用最大抗锯齿倍率: " << std::to_string(maxSamples);
    // 启用 最大 MSAA
    context()->format().setSamples(maxSamples);

    // 检查最大ubo size
    int maxUBOSize;
    GLCALL(glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUBOSize));
    qDebug() << "最大UBO块容量: " << std::to_string(maxUBOSize);

    // 标准混合模式
    GLCALL(glEnable(GL_BLEND));
    GLCALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    render = std::make_unique<Renderer2D>(this);
    render->add_texture_from_path("../resources/textures/default");

    // 暗化蒙版
    // render->newMask({0, 0, 1000, 1000}, {.2f, .2f, .2f, 0.2f},
    //                 MaskEffect::DARKEN);
    // 滤镜蒙版
    // render->newMask({0, 0, 1000, 1000}, {1.f, .5f, .2f, .75f},
    //                 MaskEffect::FILTER);
    // 透明蒙版
    render->newMask({144, 200, 388, 600}, {.3f, .5f, .2f, .75f},
                    MaskEffect::ALPHA_SHIFT);
}

void GLCanvas::resizeGL(int w, int h) {
    GLCALL(glViewport(0, 0, w, h));
    render->update_viewport({w, h});
}

void GLCanvas::paintGL() {
    auto before = std::chrono::high_resolution_clock::now().time_since_epoch();
    render->update();

    GLCALL(glClearColor(.23f, .23f, .23f, .23f));
    GLCALL(glClear(GL_COLOR_BUFFER_BIT));

    {
        // 绘制
        MPainter panter(render.get());
        panter.paintImage(
            "../resources/textures/default/物件/arrowright_selected.png",
            {50, 50}, {{1.f, 1.f}, 16.f});
        panter.fillImage(
            "../resources/textures/default/物件/arrowright_selected.png",
            {{350, 550}, {100, 100}});

        panter.paintLine({100, 50}, {200, 100}, {0.f, 0.f, 0.f, 1.f}, 4.f);
    }

    fpsCounter->frameRendered();
    pre_frame_time =
        std::chrono::high_resolution_clock::now().time_since_epoch() - before;
}
