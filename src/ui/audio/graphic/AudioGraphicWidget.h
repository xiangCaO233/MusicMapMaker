#ifndef MMM_AUDIOGRAPHICWIDGET_H
#define MMM_AUDIOGRAPHICWIDGET_H

#include <qtmetamacros.h>
#include <qwidget.h>

#include <QFuture>
#include <QFutureWatcher>
#include <QOpenGLWidget>
#include <audio/control/ProcessChain.hpp>
#include <audio/graphic/formrender/FormRenderer2D.hpp>
#include <chrono>
#include <ice/config/config.hpp>
#include <ice/core/SourceNode.hpp>
#include <ice/manage/AudioTrack.hpp>
#include <memory>

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

    // 更新当前的播放帧位置
    inline void set_currentPlaybackFrame(size_t playpos) {
        // playpos 是源音轨的帧数，我们需要把它转换成时间
        const auto source_sample_rate = static_cast<double>(
            audio_track->get_media_info().format.samplerate);

        // 使用已有的精确计算方法
        const auto duration_in_seconds = double(playpos) / source_sample_rate;
        using double_seconds = std::chrono::duration<double>;
        currentPlaybackTime =
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                double_seconds(duration_in_seconds));

        // 跟随播放逻辑
        if (followPlayback) {
            // 检查播放时间是否超出了当前视图的跟随区域
            const auto follow_point_time =
                viewStartTime +
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    visibleTimeRange * followPositionRatio);

            if (currentPlaybackTime > follow_point_time) {
                // 将视图的起始时间设置为播放指针减去跟随偏移
                viewStartTime =
                    currentPlaybackTime -
                    std::chrono::duration_cast<std::chrono::nanoseconds>(
                        visibleTimeRange * followPositionRatio);
            }
        }
        update();
    }

    // 设置当前图形类型
    inline void set_graph_type(GraphType type) {
        gtype = type;
        // 切换模式后，立即重新计算可视化
        update();
    }

    inline void set_live(bool flag) {
        renderer->set_live(flag);
        liveGraph = flag;
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

    // 实时同步处理
    bool liveGraph{false};

    // 指针是否跟随播放位置
    bool followPlayback{true};

    // 跟随位置比例
    double followPositionRatio{0.5};

    // 当前视图显示的纳秒跨度 (默认2s)
    std::chrono::nanoseconds visibleTimeRange{std::chrono::seconds(2)};

    // 当前的播放时间位置
    std::chrono::nanoseconds currentPlaybackTime{0};

    // 当前视图的起始时间
    std::chrono::nanoseconds viewStartTime{0};

    // 播放位置偏移
    std::chrono::nanoseconds timeOffset{std::chrono::milliseconds(75)};

    // 时间与帧数/像素转换的辅助函数
    inline size_t timeToFrames(std::chrono::nanoseconds t,
                               double sample_rate) const {
        if (sample_rate == 0) return 0;
        // 将纳秒转换为秒 (浮点数), 然后乘以采样率
        auto seconds = std::chrono::duration<double>(t).count();
        return size_t(std::ceil(seconds * static_cast<double>(sample_rate)));
    }

    inline std::chrono::nanoseconds framesToTime(long long f,
                                                 double sample_rate) const {
        if (sample_rate == 0) return std::chrono::nanoseconds(0);
        double seconds = static_cast<double>(f) / sample_rate;
        using double_seconds = std::chrono::duration<double>;
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            double_seconds(seconds));
    }

    inline double timeToPixels(std::chrono::nanoseconds t) const {
        if (visibleTimeRange.count() == 0) return 0;
        // 时间在视图中的比例 * 窗口宽度
        double ratio = static_cast<double>(t.count()) /
                       static_cast<double>(visibleTimeRange.count());
        return ratio * width();
    }

    inline std::chrono::nanoseconds pixelsToTime(double p) const {
        if (width() == 0) return std::chrono::nanoseconds(0);
        // 像素在窗口中的比例 * 可见时间范围
        double ratio = p / width();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            visibleTimeRange * ratio);
    }

    // 交互
    QPoint lastMousePos;
    // 拖拽
    bool isPanning{false};
};

#endif  // MMM_AUDIOGRAPHICWIDGET_H
