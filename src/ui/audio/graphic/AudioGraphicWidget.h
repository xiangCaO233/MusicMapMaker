#ifndef MMM_AUDIOGRAPHICWIDGET_H
#define MMM_AUDIOGRAPHICWIDGET_H

#include <qtmetamacros.h>
#include <qwidget.h>

#include <QFuture>
#include <QFutureWatcher>
#include <QOpenGLWidget>
#include <memory>

#include "audio/control/ProcessChain.hpp"
#include "formrender/FormRenderer2D.hpp"
#include "ice/config/config.hpp"
#include "ice/core/SourceNode.hpp"
#include "ice/manage/AudioTrack.hpp"

enum class GraphType {
    // 波形图
    WAVE,
    // 频谱图
    SPECTRO,
};

class AudioGraphicWidget : public QOpenGLWidget {
    Q_OBJECT
   public:
    // 注册到 Qt 的元对象系统
    Q_ENUM(GraphType)

    // 构造AudioGraphicWidget
    explicit AudioGraphicWidget(QWidget* parent = nullptr);

    // 析构AudioGraphicWidget
    ~AudioGraphicWidget() override;

    // 设置音轨
    void set_track(const std::shared_ptr<ice::AudioTrack>& track);

    // 获取处理链
    inline const std::shared_ptr<ProcessChain>& chain() const {
        return process_chain;
    }

    // 获取是否跟随播放指针更新
    inline bool is_follow_playback() const { return followPlayback; }

    // 设置是否跟随播放指针更新
    inline void set_follow_playback(bool flag) {
        followPlayback = flag;
        update();
    }

    // 设置跟随位置比例
    inline void set_followPositionRatio(double ratio) {
        followPositionRatio = ratio;
        update();
    }

    // 设置当前视图显示的帧数跨度(一个完整页显示的音频的帧数)
    inline void set_visibleFrameRange(size_t range) {
        visibleFrameRange = range;
    }

    // 更新当前的播放帧位置
    inline void set_currentPlaybackFrame(size_t playpos) {
        if (playpos < frameOffset) {
            currentPlaybackFrame = 0;
        } else {
            currentPlaybackFrame = playpos - frameOffset;
        }
        update();
    }

    // 设置当前图形类型
    inline void set_graph_type(GraphType type) {
        gtype = type;
        // 切换模式后，立即重新计算可视化
    }

   protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void showEvent(QShowEvent* event) override;

   private:
    // 图表渲染器(唯一)
    std::unique_ptr<FormRenderer2D> renderer;

    // 音轨引用
    std::shared_ptr<ice::AudioTrack> audio_track;

    // 独立持有sourcenode
    std::shared_ptr<ice::SourceNode> source_node;

    // 处理链(在controller被同步更新)
    std::shared_ptr<ProcessChain> process_chain;

    // 默认显示波形图
    GraphType gtype{GraphType::WAVE};

    // 指针是否跟随播放位置
    bool followPlayback{true};

    // 跟随位置比例
    double followPositionRatio{0.5};

    // 当前视图显示的帧数跨度(10s)
    size_t visibleFrameRange{ice::ICEConfig::internal_format.samplerate * 2};

    // 播放位置偏移
    size_t frameOffset{ice::ICEConfig::internal_format.samplerate * 1 / 16};

    // 当前的播放帧位置
    size_t currentPlaybackFrame{0};

    // 当前视图的起始帧
    qint64 viewStartFrame{0};

    // 交互
    QPoint lastMousePos;
    // 拖拽
    bool isPanning{false};
};

#endif  // MMM_AUDIOGRAPHICWIDGET_H
