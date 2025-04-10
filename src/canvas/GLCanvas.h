#ifndef GLCANVAS_H
#define GLCANVAS_H

#include <QtOpenGLWidgets/qopenglwidget.h>
#include <qpoint.h>

#include <memory>
#ifdef __APPLE__
#include <QtOpenGL/qopenglfunctions_4_1_core.h>
#else
#include <QtOpenGL/qopenglfunctions_4_5_core.h>
#endif  //__APPLE__
#include <qwidget.h>

#include <QMatrix4x4>

#include "renderer/RendererManager.h"

enum class TexturePoolType;
class TextureAtlas;

class GLCanvas : public QOpenGLWidget,
#ifdef __APPLE__
                 // 苹果-opengl4.1
                 public QOpenGLFunctions_4_1_Core
#else
                 // 其他-opengl4.5
                 public QOpenGLFunctions_4_5_Core
#endif  //__APPLE__
{
  friend class AbstractRenderer;
  friend class StaticRenderer;
  friend class DynamicRenderer;
  QPoint mouse_pos{0, 0};

 public:
  // 需要更新采样器uniform location
  static bool need_update_sampler_location;
  // 渲染器
  RendererManager *renderer_manager;
  // 构造GLCanvas
  explicit GLCanvas(QWidget *parent = nullptr);
  // 析构GLCanvas
  ~GLCanvas() override;

  // 当前纹理集信息
  std::shared_ptr<TextureAtlas> current_atlas;
  TexturePoolType current_atlas_used_pool;

  // 纹理映射表(id-纹理对象)
  std::unordered_map<std::string, std::shared_ptr<TextureInstace>> texture_map;

  // 从指定目录添加纹理
  void load_texture_from_path(const char *path, TexturePoolType type,
                              bool use_atlas);

  // 添加纹理
  void add_texture(const char *qrc_path, TexturePoolType type, bool use_atlas);

  // 创建新纹理池
  std::shared_ptr<BaseTexturePool> creat_new_pool(
      TexturePoolType type, QSize texture_array_used_size = {0, 0},
      bool use_atlas = false);

  // 完成纹理载入
  void finalize_texture_loading();

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
