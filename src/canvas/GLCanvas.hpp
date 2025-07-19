#ifndef MMM_GLCANVAS_HPP
#define MMM_GLCANVAS_HPP

#include <qcontainerfwd.h>
#include <qtmetamacros.h>

#include <QOpenGLFunctions_4_1_Core>
#include <QOpenGLWindow>
#include <canvas/FrameRateCounter.hpp>

class GLCanvas : public QOpenGLWindow, public QOpenGLFunctions_4_1_Core {
    Q_OBJECT
   public:
    // 构造GLCanvas
    GLCanvas();
    // 析构GLCanvas
    ~GLCanvas() override;

    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

   signals:
    void update_window_suffix(const QString &suffix);

   private:
    // fps计数器
    FrameRateCounter *fpsCounter;

    std::chrono::high_resolution_clock::duration pre_frame_time;
    long long actual_update_time;

    // 更新fps显示
    virtual void updateFpsDisplay(int fps);
};

#endif  // MMM_GLCANVAS_HPP
