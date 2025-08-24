#ifndef MMM_GLCANVAS_HPP
#define MMM_GLCANVAS_HPP

#include <QOpenGLFunctions_4_1_Core>
#include <QOpenGLWindow>
#include <canvas/FrameRateCounter.hpp>
#include <info/SharedCanvasInfo.hpp>
#include <memory>
#include <render/Renderer2D.hpp>
#include <render/synchronize/tick/RenderDataLoop.hpp>

class TexturePool;
class LayerManager;
class AudioLoadCallback;

class GLCanvas : public QOpenGLWindow, public QOpenGLFunctions_4_1_Core {
    Q_OBJECT
   public:
    // 构造GLCanvas
    GLCanvas();
    // 析构GLCanvas
    ~GLCanvas() override;

    // 绑定音频载入回调
   public slots:
    virtual void onAudioLoadcbkInitialized(AudioLoadCallback *cbk);

   signals:
    void update_window_suffix(const QString &suffix);
    // 数据循环停止信号
    void dataloop_stopped();

   protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    void keyPressEvent(QKeyEvent *e) override;
    void keyReleaseEvent(QKeyEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

    void closeEvent(QCloseEvent *e) override;

    // 更新fps显示
    virtual void updateFpsDisplay(int fps);

    // 初始化共享信息
    template <typename CanvasInfoType>
    void initSharedInfo() {
        canvas_info = std::make_unique<CanvasInfoType>();
    }

    // 共享信息
    template <typename CanvasInfoType>
    CanvasInfoType *info() {
        return static_cast<CanvasInfoType *>(canvas_info.get());
    }

    // 更新共享信息
    void update_sharedInfo() const;

    // 内部可获取渲染器
    std::unique_ptr<Renderer2D> &renderer() { return render; }

    // 内部可获取纹理加载回调
    TextureLoadCallback *textureCallback() const { return render.get(); }

    // 内部可获取音频加载回调
    AudioLoadCallback *audioLoadCallback() const { return audioLoadcbk; }

    // 内部可获取数据循环
    std::unique_ptr<RenderDataLoop> &dataloop() { return render_dataloop; }

    // 内部可获取目标fps
    qreal desired_fps() const { return desiredFps; }

    // 内部可获取fps计数器
    FrameRateCounter *fps_counter() { return fpsCounter; }

   private:
    // fps计数器
    FrameRateCounter *fpsCounter;

    // 持有主渲染器
    std::unique_ptr<Renderer2D> render;

    // 持有数据循环
    std::unique_ptr<RenderDataLoop> render_dataloop;

    // 持有共享画布信息(子类自己初始化)
    std::unique_ptr<SharedCanvasInfo> canvas_info{nullptr};

    // 音频加载回调指针
    AudioLoadCallback *audioLoadcbk;

    // 上一帧的时间
    std::chrono::high_resolution_clock::duration pre_frame_time;

    // 目标帧率
    qreal desiredFps;

    long long m_last_paint_time_us{0};

    // 实际update处理时间
    long long last_update_time_us{0};

    friend class ProjectService;
};

#endif  // MMM_GLCANVAS_HPP
