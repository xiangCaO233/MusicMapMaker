#ifndef GLCANVAS_H
#define GLCANVAS_H

#include <QtOpenGL/qopenglfunctions_4_1_core.h>
#include <QtOpenGLWidgets/qopenglwidget.h>
#include <qwidget.h>

#include "renderer/RendererManager.h"

class GLCanvas : public QOpenGLWidget, QOpenGLFunctions_4_1_Core {
  friend class AbstractRenderer;
  friend class StaticRenderer;
  friend class DynamicRenderer;

 public:
  // 渲染器
  RendererManager *renderer_manager;
  // 构造GLCanvas
  explicit GLCanvas(QWidget *parent = nullptr);
  // 析构GLCanvas
  ~GLCanvas() override;

 protected:
  void initializeGL() override;
  void resizeGL(int w, int h) override;
  void paintGL() override;

  // qt事件
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void keyReleaseEvent(QKeyEvent *event) override;
  void focusInEvent(QFocusEvent *event) override;
  void focusOutEvent(QFocusEvent *event) override;
  void enterEvent(QEnterEvent *event) override;
  void leaveEvent(QEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;
};

#endif  // GLCANVAS_H
