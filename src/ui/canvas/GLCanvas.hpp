#ifndef MMM_GLCANVAS_HPP
#define MMM_GLCANVAS_HPP

#include <QOpenGLFunctions_4_1_Core>
#include <QOpenGLWindow>
#include <canvas/FrameRateCounter.hpp>
#include <memory>
#include <render/Renderer2D.hpp>

class TexturePool;
class GLCanvas : public QOpenGLWindow, public QOpenGLFunctions_4_1_Core {
    Q_OBJECT
   public:
    // 构造GLCanvas
    GLCanvas();
    // 析构GLCanvas
    ~GLCanvas() override;

   signals:
    void update_window_suffix(const QString &suffix);

   protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    // 更新fps显示
    virtual void updateFpsDisplay(int fps);

    // 内部可获取渲染器
    std::unique_ptr<Renderer2D> &renderer() { return render; }

   private:
    // fps计数器
    FrameRateCounter *fpsCounter;

    // 主渲染器
    std::unique_ptr<Renderer2D> render;

    std::chrono::high_resolution_clock::duration pre_frame_time;

    long long actual_update_time;
};

#endif  // MMM_GLCANVAS_HPP
