#ifndef GLRENDERER_H
#define GLRENDERER_H

#include <qcolor.h>
#include <qrect.h>

#include <cstdint>
#include <map>
#include <vector>

#include "RenderCommand.h"
#include "TexturePool.h"

struct DrawBatch {
  // 纹理池id
  int32_t texture_pool_id;
  // 使用此纹理池id的全部渲染指令
  std::vector<RenderCommand> commands;
};

class GLRenderer {
  // 纹理池表
  std::map<int32_t, TexturePool> texture_pools;
  // 所有渲染批(纹理池id-渲染批/纹理id=-1时填充纯色)
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
