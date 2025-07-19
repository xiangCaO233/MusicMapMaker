#include <QGuiApplication>
#include <QScreen>
#include <canvas/GLCanvas.hpp>
#include <chrono>
#include <type_traits>
#include <utility>

// 使用 C++17 的 if constexpr 的模板帮助函数
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

    // 查询最大支持多层纹理的最大层数
    GLint maxLayers;
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maxLayers);
    qDebug() << "多层纹理最大层数: " << std::to_string(maxLayers);

    // 查询纹理采样器最大连续数量
    GLint max_fragment_samplers;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_fragment_samplers);
    qDebug() << "纹理采样器最大连续数量: "
             << std::to_string(max_fragment_samplers / 2);
    if (max_fragment_samplers > 16) {
        max_fragment_samplers = 16;
    }

    // 查询纹理采样器最大数量
    GLint max_combined_samplers;
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_combined_samplers);
    qDebug() << "纹理采样器最大数量: " << std::to_string(max_combined_samplers);

    // 查询最大支持抗锯齿MSAA倍率
    GLint maxSamples;
    GLCALL(glGetIntegerv(GL_MAX_SAMPLES, &maxSamples));

    // 初始化纹理池驱动信息
    // MTexturePool::init_driver_info(maxLayers, max_combined_samplers,
    //                                max_fragment_samplers);

    qDebug() << "启用最大抗锯齿倍率: " << std::to_string(maxSamples);
    // 启用 最大 MSAA
    context()->format().setSamples(maxSamples);

    // XINFO("启用垂直同步");

    // 检查最大ubo size
    int maxUBOSize;
    GLCALL(glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUBOSize));
    qDebug() << "最大UBO块容量: " << std::to_string(maxUBOSize);

    // 标准混合模式
    GLCALL(glEnable(GL_BLEND));
    GLCALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
}

void GLCanvas::resizeGL(int w, int h) { GLCALL(glViewport(0, 0, w, h)); }

void GLCanvas::paintGL() {
    auto before = std::chrono::high_resolution_clock::now().time_since_epoch();
    GLCALL(glClearColor(1.f, 1.f, 1.f, 1.f));
    GLCALL(glClear(GL_COLOR_BUFFER_BIT));
    pre_frame_time =
        std::chrono::high_resolution_clock::now().time_since_epoch() - before;
    fpsCounter->frameRendered();
}
