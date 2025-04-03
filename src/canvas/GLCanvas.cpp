#include "GLCanvas.h"

#include <qlogging.h>

#include <QFile>
#include <QMouseEvent>
#include <QTextStream>
#include <string>

#include "colorful-log.h"

// 用于包装 OpenGL 调用并检查错误
#define GLCALL(func)                                       \
  func;                                                    \
  {                                                        \
    GLenum error = glGetError();                           \
    if (error != GL_NO_ERROR) {                            \
      XERROR("在[" + std::string(#func) +                  \
             "]发生OpenGL错误: " + std::to_string(error)); \
    }                                                      \
  }

GLCanvas::GLCanvas(QWidget *parent) : QOpenGLWidget(parent) {
  // 启用鼠标跟踪
  setMouseTracking(true);
}

GLCanvas::~GLCanvas() {
  // 释放渲染管理器
  delete renderer_manager;
};

// qt事件
// 鼠标按下事件
void GLCanvas::mousePressEvent(QMouseEvent *event) {
  // 传递事件
  QOpenGLWidget::mousePressEvent(event);
}

// 鼠标释放事件
void GLCanvas::mouseReleaseEvent(QMouseEvent *event) {
  // 传递事件
  QOpenGLWidget::mouseReleaseEvent(event);
}

// 鼠标双击事件
void GLCanvas::mouseDoubleClickEvent(QMouseEvent *event) {
  // 传递事件
  QOpenGLWidget::mouseDoubleClickEvent(event);
}

// 鼠标移动事件
void GLCanvas::mouseMoveEvent(QMouseEvent *event) {
  // 传递事件
  QOpenGLWidget::mouseMoveEvent(event);
}

// 鼠标滚动事件
void GLCanvas::wheelEvent(QWheelEvent *event) {
  // 传递事件
  QOpenGLWidget::wheelEvent(event);
}

// 键盘按下事件
void GLCanvas::keyPressEvent(QKeyEvent *event) {
  // 传递事件
  QOpenGLWidget::keyPressEvent(event);
}

// 键盘释放事件
void GLCanvas::keyReleaseEvent(QKeyEvent *event) {
  // 传递事件
  QOpenGLWidget::keyReleaseEvent(event);
}

// 取得焦点事件
void GLCanvas::focusInEvent(QFocusEvent *event) {
  // 传递事件
  QOpenGLWidget::focusInEvent(event);
}

// 失去焦点事件
void GLCanvas::focusOutEvent(QFocusEvent *event) {
  // 传递事件
  QOpenGLWidget::focusOutEvent(event);
}

// 进入事件
void GLCanvas::enterEvent(QEnterEvent *event) {
  // 传递事件
  QOpenGLWidget::enterEvent(event);
}

// 退出事件
void GLCanvas::leaveEvent(QEvent *event) {
  // 传递事件
  QOpenGLWidget::leaveEvent(event);
}

// 调整尺寸事件
void GLCanvas::resizeEvent(QResizeEvent *event) {
  // 传递事件
  QOpenGLWidget::resizeEvent(event);
}

void GLCanvas::initializeGL() {
  initializeOpenGLFunctions();
  // 初始化渲染管理器
  renderer_manager = new RendererManager(context()->functions());
  // 标准混合模式
  GLCALL(glEnable(GL_BLEND));
  GLCALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
}
void GLCanvas::resizeGL(int w, int h) { glViewport(0, 0, w, h); }

// 绘制画布
void GLCanvas::paintGL() {
  // 背景色
  GLCALL(glClearColor(0.23f, 0.23f, 0.23f, 1.0f));
  GLCALL(glClear(GL_COLOR_BUFFER_BIT));

  renderer_manager->renderAll();
  // 绘制矩形
  // GLCALL(glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, 1));
  // 绘制椭圆
  // GLCALL(glDrawArraysInstanced(GL_TRIANGLE_FAN, 4, oval_segment, 1));
}

// 初始化缓冲区
void GLCanvas::initbuffer() {}

// 初始化着色器程序
void GLCanvas::initshader() {}
