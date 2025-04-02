#ifndef GLRENDERER_H
#define GLRENDERER_H

#include <QtOpenGL/qopenglfunctions_4_1_core.h>
#include <qcolor.h>
#include <qrect.h>

#include <cstdint>
#include <map>
#include <vector>

#include "../../texture/TexturePool.h"
#include "../RenderCommand.h"

class GLCanvas;

class StaticRenderer : public QOpenGLFunctions_4_1_Core {
  // 顶点数组对象
  GLuint VAO;
  // 顶点缓冲对象
  GLuint VBO;
  GLuint instanceBO;
  GLuint EBO;
  // 帧缓冲对象
  GLuint FBO;
  // 统一缓冲对象
  GLuint UBO;
  // 着色器程序
  GLuint shader_program;

  // 纹理池表
  std::map<int32_t, TexturePool> texture_pools;
  // 渲染批列表
  std::vector<DrawBatch*> batches;
  // 提交当前批次
  void submitBatch();

  friend class GLCanvas;

 public:
  // 构造GLRenderer
  StaticRenderer();
  // 析构GLRenderer
  virtual ~StaticRenderer();

  // 绑定渲染器
  void bind();
  // 解除绑定渲染器
  void unbind();

  // 渲染向此渲染器提交的全部图形
  void render();

  // 添加矩形
  void addRect(const QRectF& rect, uint32_t textureId,
               const QColor& fill_color);
  // 添加椭圆
  void addEllipse(const QRectF& bounds, uint32_t textureId,
                  const QColor& fill_color);
};

#endif  // GLRENDERER_H
