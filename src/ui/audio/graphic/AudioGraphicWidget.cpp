#include <AudioGraphicWidget.h>

#include "audio/control/ProcessChain.hpp"
#include "ice/core/SourceNode.hpp"

// 构造AudioGraphicWidget
AudioGraphicWidget::AudioGraphicWidget(QWidget* parent) : QWidget(parent) {
    // 创建渲染器实例
    waveformRenderer = std::make_unique<WaveformRenderer>();
    spectrogramRenderer = std::make_unique<SpectrogramRenderer>();

    // 默认使用波形图渲染器
    currentRenderer = waveformRenderer.get();
}

// 析构AudioGraphicWidget
AudioGraphicWidget::~AudioGraphicWidget() = default;

// 设置音轨
void AudioGraphicWidget::set_track(
    const std::shared_ptr<ice::AudioTrack>& track) {
    audio_track = track;

    // 创建一个独立的source
    source_node = std::make_shared<ice::SourceNode>(track);

    // 构建一个处理链
    process_chain = std::make_unique<ProcessChain>(source_node);
    if (!isHidden()) {
        updateVisualization();
    }
}

// 重写事件
void AudioGraphicWidget::paintEvent(QPaintEvent* e) {
    QPainter painter(this);
    if (!currentRenderer) return;
    currentRenderer->paint(&painter, rect());

    size_t totalFrames =
        process_chain ? audio_track->get_media_info().frame_count : 0;
    size_t startFrame = 0;
    if (followPlayback) {
        size_t frames_on_left = visibleFrameRange * followPositionRatio;
        startFrame = (currentPlaybackFrame > frames_on_left)
                         ? currentPlaybackFrame - frames_on_left
                         : 0;
    }
    if (startFrame + visibleFrameRange > totalFrames) {
        startFrame = (totalFrames > visibleFrameRange)
                         ? totalFrames - visibleFrameRange
                         : 0;
    }

    int playhead_x = -1;
    if (followPlayback) {
        playhead_x = width() * followPositionRatio;
    } else {
        if (currentPlaybackFrame >= startFrame &&
            currentPlaybackFrame < startFrame + visibleFrameRange) {
            playhead_x = width() * (double(currentPlaybackFrame - startFrame) /
                                    visibleFrameRange);
        }
    }
    if (playhead_x >= 0) {
        painter.setPen(QPen(Qt::red, 2));
        painter.drawLine(playhead_x, 0, playhead_x, height());
    }
}

// 更新可视化
void AudioGraphicWidget::updateVisualization() {
    if (!process_chain || !currentRenderer) return;
    size_t totalFrames = audio_track->get_media_info().frame_count;
    size_t startFrame = 0;
    if (followPlayback) {
        size_t frames_on_left = visibleFrameRange * followPositionRatio;
        startFrame = (currentPlaybackFrame > frames_on_left)
                         ? currentPlaybackFrame - frames_on_left
                         : 0;
    }
    if (startFrame + visibleFrameRange > totalFrames) {
        startFrame = (totalFrames > visibleFrameRange)
                         ? totalFrames - visibleFrameRange
                         : 0;
    }
    currentRenderer->process(process_chain.get(), audio_track, startFrame,
                             visibleFrameRange);
    update();
}
