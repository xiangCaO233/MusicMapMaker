#include <audio/graphic/AudioGraphicWidget.h>

#include <QPainter>
#include <QtConcurrent>
#include <audio/control/ProcessChain.hpp>
#include <audio/graphic/formrender/FormRenderer2D.hpp>
#include <ice/config/config.hpp>
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

    // 计算引擎需要渲染多少帧 (基于引擎采样率)
    const double engine_sample_rate =
        ice::ICEConfig::internal_format.samplerate;
    const auto engine_visible_frames =
        timeToFrames(visibleTimeRange, engine_sample_rate);

    // 设置正交投影矩阵 (X轴现在代表引擎处理的帧数)
    QMatrix4x4 projection;
    const QMatrix4x4 view;

    // 计算SourceNode应该从哪里开始读取 (基于源采样率)
    const double source_sample_rate =
        audio_track->get_media_info().format.samplerate;
    const auto source_start_frame =
        timeToFrames(viewStartTime, source_sample_rate);
    projection.ortho(0.0f, static_cast<float>(engine_visible_frames), -1.f, 1.f,
                     -1.f, 1.f);

    if (liveGraph) {
        // 读取音频数据到缓冲区
        renderer->wav().resize(ice::ICEConfig::internal_format,
                               size_t(std::ceil(engine_visible_frames)));
        process_chain->source->set_playpos(
            size_t(std::ceil(source_start_frame)));
        process_chain->source->play();
        process_chain->output->process(renderer->wav());
        process_chain->source->pause();
    } else {
        renderer->span().clear();
        // const auto source_visible_frames =
        //     timeToFrames(visibleTimeRange, source_sample_rate);
        // projection.ortho(0.0f, static_cast<float>(source_visible_frames),
        // -1.f,
        //                  1.f, -1.f, 1.f);
        audio_track->origin(renderer->span(), source_start_frame,
                            engine_visible_frames);
    }

    // 渲染图形
    renderer->render(gtype, projection, view);

    // 绘制播放指针 (完全基于时间)
    // QPainter 叠加绘制
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing);
    if (currentPlaybackTime >= viewStartTime &&
        currentPlaybackTime < viewStartTime + visibleTimeRange) {
        // 计算播放指针相对于视图起始点的时间差
        const auto time_offset =
            currentPlaybackTime - viewStartTime - timeOffset;
        // 将时间差转换为像素位置
        const double x_pos = timeToPixels(time_offset);

        painter.setPen(QPen(Qt::red, 1.5));
        painter.drawLine(QPointF(x_pos, 0), QPointF(x_pos, height()));
    }
    painter.end();
}
