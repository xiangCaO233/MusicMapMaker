#ifndef MMM_GLCANVAS_HPP
#define MMM_GLCANVAS_HPP

#include <QOpenGLFunctions_4_1_Core>
#include <QOpenGLWindow>
#include <canvas/FrameRateCounter.hpp>
#include <info/SharedCanvasInfo.hpp>
#include <memory>
#include <mmm/project/TextureLoadCallback.hpp>
#include <render/Renderer2D.hpp>
#include <render/synchronize/RenderDataLoop.hpp>

class TexturePool;
class LayerManager;
class GLCanvas : public QOpenGLWindow,
                 public QOpenGLFunctions_4_1_Core,
                 public TextureLoadCallback {
    Q_OBJECT
   public:
    // 构造GLCanvas
    GLCanvas();
    // 析构GLCanvas
    ~GLCanvas() override;

    // 需要载入纹理
    void need_loadtexture_dir(std::string_view texdir) override;
    // 需要卸载纹理
    void need_unloadtexture_dir(std::string_view texdir) override;

   signals:
    void update_window_suffix(const QString &suffix);

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
    template <typename InfoType>
    void initSharedInfo() {
        canvas_info = std::make_unique<InfoType>();
    }

    // 内部可获取渲染器
    std::unique_ptr<Renderer2D> &renderer() { return render; }

   private:
    // fps计数器
    FrameRateCounter *fpsCounter;

    // 持有主渲染器
    std::unique_ptr<Renderer2D> render;

    // 持有数据循环
    std::unique_ptr<RenderDataLoop> render_dataloop;

    // 持有共享画布信息(子类自己初始化)
    std::unique_ptr<SharedCanvasInfo> canvas_info{nullptr};

    // 上一帧的时间
    std::chrono::high_resolution_clock::duration pre_frame_time;

    // 目标帧率
    qreal desiredFps;

    // 实际update处理时间
    long long actual_update_time;
};

#endif  // MMM_GLCANVAS_HPP
