#include <AudioGraphicWidget.h>
#include <qlogging.h>

#include <QtConcurrent>
#include <chrono>

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
    viewStartTime = std::chrono::nanoseconds(0);

    // 创建一个独立的source
    source_node = std::make_shared<ice::SourceNode>(track);

    // 恢复原始音量
    source_node->setvolume(1.f);

    // 构建一个处理链
    process_chain = std::make_shared<ProcessChain>(source_node);

    // 只使用eq
    process_chain->eq->set_inputnode(process_chain->source);
    process_chain->output = process_chain->eq;

    double offset = 75. * (double(ice::ICEConfig::internal_format.samplerate) /
                           double(track->get_media_info().format.samplerate));

    // 更新偏移
    timeOffset = std::chrono::nanoseconds(size_t(offset * 1000000));

    if (!isHidden()) {
        update();
    }
}
