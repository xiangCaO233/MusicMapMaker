#ifndef GLCANVAS_H
#define GLCANVAS_H

#include <GL/gl.h>
#include <QtOpenGL/qopenglfunctions_4_1_core.h>
#include <QtOpenGLWidgets/qopenglwidget.h>
#include <qwidget.h>

class GLCanvas : public QOpenGLWidget, QOpenGLFunctions_4_1_Core {
 public:
  // 构造GLCanvas
  explicit GLCanvas(QWidget *parent = nullptr);
  // 析构GLCanvas
  ~GLCanvas() override;

 protected:
  void initializeGL() override;
  void resizeGL(int w, int h) override;
  void paintGL() override;
};

#endif  // GLCANVAS_H
