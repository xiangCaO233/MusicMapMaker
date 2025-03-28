#ifndef GLCANVAS_H
#define GLCANVAS_H

#include <GL/gl.h>
#include <QtOpenGL/qopenglfunctions_4_1_core.h>
#include <QtOpenGLWidgets/qopenglwidget.h>
#include <qwidget.h>

#include <cstdint>

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

 private:
  GLuint VAO;
  GLuint VBO;
  GLuint instanceBO;
  GLuint EBO;
  GLuint FBO;
  GLuint UBO;
  GLuint shader_program;

  // 椭圆分割精度
  int32_t oval_segment{64};

  void initbuffer();
  void initshader();
};

#endif  // GLCANVAS_H
