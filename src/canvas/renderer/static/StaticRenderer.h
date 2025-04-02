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
  // 纹理池表
  std::map<int32_t, TexturePool> texture_pools;
  // 渲染批列表
  std::vector<DrawBatch*> batches;

  friend class GLCanvas;

 public:
  // 构造GLRenderer
  StaticRenderer();
  // 析构GLRenderer
  virtual ~StaticRenderer();

  // 添加矩形
  void addRect(const QRectF& rect, uint32_t textureId,
               const QColor& fill_color);
  // 添加椭圆
  void addEllipse(const QRectF& bounds, uint32_t textureId,
                  const QColor& fill_color);
  // 提交当前批次
  void submitBatch();
};

#endif  // GLRENDERER_H
