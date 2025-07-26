#ifndef MMM_AUDIOGRAPHICWIDGET_H
#define MMM_AUDIOGRAPHICWIDGET_H

#include <qtmetamacros.h>
#include <qwidget.h>

#include <QFuture>
#include <QFutureWatcher>
#include <QOpenGLFunctions_4_1_Core>
#include <QOpenGLWidget>
#include <memory>
#include <vector>

#include "audio/control/ProcessChain.hpp"
#include "ice/core/SourceNode.hpp"
#include "ice/manage/AudioTrack.hpp"

class AudioGraphicWidget : public QOpenGLWidget,
                           public QOpenGLFunctions_4_1_Core {
    // 存储每个像素列的振幅最大最小值
    struct MinMaxSample {
        float min = 1.0f;
        float max = -1.0f;
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

    // 设置跟随位置比例
    inline void set_followPositionRatio(double ratio) {
        followPositionRatio = ratio;
        update();
    }

    // 设置当前视图显示的帧数跨度(一个完整页显示的音频的帧数)
    inline void set_visibleFrameRange(size_t range) {
        visibleFrameRange = range;
        updateVisualization();
    }

    // 更新当前的播放帧位置
    inline void set_currentPlaybackFrame(size_t playpos) {
        currentPlaybackFrame = playpos;
        if (followPlayback) {
            // 计算理论上播放指针应该在的起始帧，以使其保持在
            // followPositionRatio 的位置
            qint64 desiredStartFrame =
                currentPlaybackFrame -
                (visibleFrameRange * followPositionRatio);

            // 防止计算出的起始帧为负数
            if (desiredStartFrame < 0) {
                desiredStartFrame = 0;
            }
            updateVisualization();
            update();
        } else {
            // 视图是固定的，只需要重绘来更新红色播放指针的位置
            update();
        }
    }

    // 设置当前图形类型
    inline void set_graph_type(GraphType type) {
        gtype = type;

        // 切换模式后，立即重新计算可视化
        updateVisualization();
    }

   signals:
    // 当缓存计算完成时，由后台线程发出，通知主线程
    void cacheUpdated(const std::vector<MinMaxSample>& newCache);

   protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

   private:
    // 音轨引用
    std::shared_ptr<ice::AudioTrack> audio_track;

    // 独立持有sourcenode
    std::shared_ptr<ice::SourceNode> source_node;

    // 处理链(在controller被同步更新)
    std::unique_ptr<ProcessChain> process_chain;

    // 默认显示波形图
    GraphType gtype{GraphType::WAVE};

    // 指针是否跟随播放位置
    bool followPlayback{true};

    // 跟随位置比例
    double followPositionRatio{0.5};

    // 当前视图显示的帧数跨度
    size_t visibleFrameRange{2048};

    // 当前的播放帧位置
    size_t currentPlaybackFrame{0};

    // 当前视图的起始帧
    qint64 viewStartFrame{0};

    // 这个函数现在是后台线程执行的核心逻辑
    // 它只负责计算，并返回结果
    std::vector<MinMaxSample> calculateWaveform(qint64 startFrame, size_t range,
                                                int numSamples);

    // 异步计算的管理
    QFuture<std::vector<MinMaxSample>> cacheFuture;
    QFutureWatcher<std::vector<MinMaxSample>> cacheWatcher;

    // 是否正在计算
    bool isCalculating{false};

    // 更新可视化
    void updateVisualization();

    // 绘制波形图 从缓存绘制
    void drawWaveform(QPainter& painter);

    // 绘制频谱图
    void drawSpectrogram(QPainter& painter);

    // 波形图数据缓存
    std::vector<MinMaxSample> waveformCache;
};

#endif  // MMM_AUDIOGRAPHICWIDGET_H
