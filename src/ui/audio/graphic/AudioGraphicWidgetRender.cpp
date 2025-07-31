#include <audio/graphic/AudioGraphicWidget.h>

#include <QPainter>
#include <QtConcurrent>
#include <audio/control/ProcessChain.hpp>
#include <audio/graphic/formrender/FormRenderer2D.hpp>
#include <ice/core/SourceNode.hpp>

// C++17 的 if constexpr 的模板帮助函数
template <typename Func>
auto glCallImpl(Func func, const char* funcStr) {
    // 对 lambda 本身的返回类型进行判断
    if constexpr (std::is_void_v<decltype(func())>) {
        // lambda 返回 void
        // 调用 lambda
        func();
        if (const GLenum error = glGetError() != GL_NO_ERROR) {
            qDebug() << "OpenGL Error in [" << funcStr << "]: " << error;
        }
        // 此分支无返回
    } else {
        // lambda 有返回值
        // 调用 lambda 并捕获结果
        auto&& result = func();
        if (const GLenum error = glGetError() != GL_NO_ERROR) {
            qDebug() << "OpenGL Error in [" << funcStr << "]: " << error;
        }
        // 返回结果
        return std::forward<decltype(result)>(result);
    }
}

// 用于包装 OpenGL 调用并检查错误
#define GLCALL(func) glCallImpl([&]() { return func; }, #func)

void AudioGraphicWidget::initializeGL() {
    // 初始化gl资源
    renderer = std::make_unique<FormRenderer2D>(this);
}

void AudioGraphicWidget::resizeGL(int w, int h) {
    GLCALL(glViewport(0, 0, w, h));
    if (renderer) {
        renderer->resize(w, h);
    }
    // 尺寸变化意味着每像素代表的帧数变了，需要重新计算
    update();
}

void AudioGraphicWidget::paintGL() {
    GLCALL(glClearColor(.23f, .23f, .23f, 1.f));
    GLCALL(glClear(GL_COLOR_BUFFER_BIT));
    if (!renderer || !audio_track) return;

    // OpenGL 渲染
    QMatrix4x4 projection;
    // Y轴从-1.0到1.0代表最大振幅
    projection.ortho(0.0f, static_cast<float>(visibleFrameRange), -1.0f, 1.0f,
                     -1.0f, 1.0f);
    const QMatrix4x4 view;
    // view 矩阵将世界坐标（像素索引）映射到屏幕
    // 在着色器中用 gl_VertexID 作为x坐标，所以不需要平移和缩放
    // 真正的平移缩放体现在我们从哪个源数据点开始计算

    // 读取音频数据到缓冲区
    renderer->wav().resize(source_node->format(), visibleFrameRange);

    process_chain->source->set_playpos(viewStartFrame);

    process_chain->source->play();
    process_chain->output->process(renderer->wav());
    process_chain->source->pause();

    renderer->render(gtype, projection, view);

    // --- QPainter 叠加绘制 ---
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing);

    // 绘制播放指针
    if (currentPlaybackFrame >= viewStartFrame &&
        currentPlaybackFrame < viewStartFrame + visibleFrameRange) {
        const double framesPerPixel =
            static_cast<double>(visibleFrameRange) / width();
        const double x_pos =
            (static_cast<double>(currentPlaybackFrame) - viewStartFrame) /
            framesPerPixel;
        painter.setPen(QPen(Qt::red, 1.5));
        painter.drawLine(QPointF(x_pos, 0), QPointF(x_pos, height()));
    }
    painter.end();
}
