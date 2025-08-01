#include <AudioGraphicWidget.h>

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

    // 恢复原始音量
    source_node->setvolume(1.f);

    // 构建一个处理链
    process_chain = std::make_shared<ProcessChain>(source_node);

    // 只使用eq
    process_chain->eq->set_inputnode(process_chain->source);
    process_chain->output = process_chain->eq;

    if (!isHidden()) {
        update();
    }
}
