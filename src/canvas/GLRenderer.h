#ifndef GLRENDERER_H
#define GLRENDERER_H

#include <qrect.h>

#include <cstdint>
#include <map>
#include <vector>

#include "RenderCommand.h"

struct DrawBatch {
  int32_t texture_id;
  std::vector<RenderCommand> commands;
};

class GLRenderer {
  // 所有渲染批(纹理id-渲染批/纹理id=-1时填充纯色)
  std::map<int32_t, DrawBatch> batches;

 public:
  // 构造GLRenderer
  GLRenderer();
  // 析构GLRenderer
  virtual ~GLRenderer();

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
