#ifndef GLCANVAS_H
#define GLCANVAS_H

#include <QtOpenGLWidgets/qopenglwidget.h>
#ifdef __APPLE__
#include <QtOpenGL/qopenglfunctions_4_1_core.h>
#else
#include <QtOpenGL/qopenglfunctions_4_5_core.h>
#endif  //__APPLE__
#include <qwidget.h>

#include <QMatrix4x4>

#include "renderer/RendererManager.h"

enum class TexturePoolType;

class GLCanvas : public QOpenGLWidget,
#ifdef __APPLE__
                 public QOpenGLFunctions_4_1_Core
#else
                 public QOpenGLFunctions_4_5_Core
#endif  //__APPLE__
{
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

  // 添加纹理
  void add_texture(const char *qrc_path, TexturePoolType type);

  // 设置垂直同步
  void set_Vsync(bool flag);

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
