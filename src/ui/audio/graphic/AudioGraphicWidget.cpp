#include <AudioGraphicWidget.h>
#include <GL/gl.h>
#include <qmatrix4x4.h>
#include <qopenglvertexarrayobject.h>

#include <QPainter>
#include <QtConcurrent>

#include "audio/control/ProcessChain.hpp"
#include "ice/core/SourceNode.hpp"

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

// 构造AudioGraphicWidget
AudioGraphicWidget::AudioGraphicWidget(QWidget* parent)
    : QOpenGLWidget(parent) {}

// 析构AudioGraphicWidget
AudioGraphicWidget::~AudioGraphicWidget() {}

// 设置音轨
void AudioGraphicWidget::set_track(
    const std::shared_ptr<ice::AudioTrack>& track) {
    audio_track = track;

    // 新轨道从头开始
    viewStartFrame = 0;

    // 创建一个独立的source
    source_node = std::make_shared<ice::SourceNode>(track);

    // 构建一个处理链
    process_chain = std::make_unique<ProcessChain>(source_node);

    // 只使用eq
    process_chain->eq->set_inputnode(process_chain->source);
    process_chain->output = process_chain->eq;

    if (!isHidden()) {
        updateVisualization();
    }
}

void AudioGraphicWidget::initializeGL() {
    initializeOpenGLFunctions();
    // 初始化gl资源
}

void AudioGraphicWidget::resizeGL(int w, int h) {
    GLCALL(glViewport(0, 0, w, h));
    // 更新视图矩阵(如果需要)
}

void AudioGraphicWidget::paintGL() {
    GLCALL(glClearColor(.23f, .23f, .23f, 1.f));
    GLCALL(glClear(GL_COLOR_BUFFER_BIT));
}

// 更新可视化
void AudioGraphicWidget::updateVisualization() {}

std::vector<AudioGraphicWidget::MinMaxSample>
AudioGraphicWidget::calculateWaveform(qint64 startFrame, size_t range,
                                      int numSamples) {}

// 绘制波形图 从缓存绘制
void AudioGraphicWidget::drawWaveform(QPainter& painter) {}

// 绘制频谱图
void AudioGraphicWidget::drawSpectrogram(QPainter& painter) {}
