#ifndef MMM_AUDIOGRAPHICWIDGET_H
#define MMM_AUDIOGRAPHICWIDGET_H

#include <qtmetamacros.h>
#include <qwidget.h>

#include <memory>

#include "audio/control/ProcessChain.hpp"
#include "ice/core/SourceNode.hpp"
#include "ice/manage/AudioTrack.hpp"
#include "render/SpectrogramRenderer.hpp"
#include "render/WaveformRenderer.hpp"

class AudioGraphicWidget : public QWidget {
    // 存储完整的波形缓存
    struct WaveformCache {
        // 每个声道的处理后的浮点数据
        QVector<QVector<float>> channelData;
        // 声道数
        int channelCount = 0;
    };
    Q_OBJECT
   public:
    enum class GraphType {
        // 波形图
        WAVE,
        // 频谱图
        SPECTRO,
    };
    // 注册到 Qt 的元对象系统
    Q_ENUM(GraphType)

    // 构造AudioGraphicWidget
    explicit AudioGraphicWidget(QWidget* parent = nullptr);

    // 析构AudioGraphicWidget
    ~AudioGraphicWidget() override;

    // 设置音轨
    void set_track(const std::shared_ptr<ice::AudioTrack>& track);

    // 获取处理链
    inline const std::unique_ptr<ProcessChain>& chain() const {
        return process_chain;
    }

    // 获取是否跟随播放指针更新
    inline bool is_follow_playback() const { return followPlayback; }

    // 设置是否跟随播放指针更新
    inline void set_follow_playback(bool flag) {
        followPlayback = flag;
        update();
    }

    // 设置跟随位置比例大小
    inline void set_followPositionRatio(double ratio) {
        followPositionRatio = ratio;
        update();
    }

    // 设置当前视图显示的帧数跨度
    inline void set_visibleFrameRange(size_t range) {
        visibleFrameRange = range;
        updateVisualization();
    }

    // 更新当前的播放帧位置
    inline void set_currentPlaybackFrame(size_t playpos) {
        currentPlaybackFrame = playpos;
        if (followPlayback) {
            updateVisualization();
        } else {
            update();
        }
    }

    // 设置当前图形类型
    inline void set_graph_type(GraphType type) {
        gtype = type;
        if (type == GraphType::WAVE) {
            currentRenderer = waveformRenderer.get();
        } else if (type == GraphType::SPECTRO) {
            currentRenderer = spectrogramRenderer.get();
        }

        // 切换模式后，立即重新计算可视化
        updateVisualization();
    }

   protected:
    // 重写repaint
    void paintEvent(QPaintEvent* e) override;

   private:
    // 音轨引用
    std::shared_ptr<ice::AudioTrack> audio_track;

    // 独立持有sourcenode
    std::shared_ptr<ice::SourceNode> source_node;

    // 处理链(在controller被同步更新)
    std::unique_ptr<ProcessChain> process_chain;

    // 默认显示波形图
    GraphType gtype{GraphType::WAVE};

    // 所有渲染器
    std::unique_ptr<WaveformRenderer> waveformRenderer;
    std::unique_ptr<SpectrogramRenderer> spectrogramRenderer;

    // 当前活动渲染器的指针
    IRenderer* currentRenderer;

    // 波形缓存数据
    WaveformCache waveformCache;

    // 指针是否跟随播放位置
    bool followPlayback{true};

    // 跟随位置比例
    double followPositionRatio{0.5};

    // 当前视图显示的帧数跨度
    size_t visibleFrameRange{1024};

    // 当前的播放帧位置
    size_t currentPlaybackFrame{0};

    // 更新可视化
    void updateVisualization();
};

#endif  // MMM_AUDIOGRAPHICWIDGET_H
