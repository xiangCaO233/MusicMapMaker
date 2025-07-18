#ifndef MMM_GLCANVAS_HPP
#define MMM_GLCANVAS_HPP

#include <QOpenGLFunctions_4_1_Core>
#include <QOpenGLWindow>

class GLCanvas : public QOpenGLWindow, public QOpenGLFunctions_4_1_Core {
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
};

#endif  // MMM_GLCANVAS_HPP
