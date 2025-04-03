#ifndef RENDERER_MANAGER_H
#define RENDERER_MANAGER_H

#include <queue>

#include "../texture/TexturePool.h"
#include "dynamic/DynamicRenderer.h"
#include "static/StaticRenderer.h"

class RendererManager {
  // 纹理池表
  std::map<int32_t, TexturePool> texture_pools;
  // 是否处理好准备渲染
  bool is_ready{false};
  // 渲染器
  StaticRenderer* static_renderer;
  DynamicRenderer* dynamic_renderer;

  // 渲染指令队列
  std::queue<RenderCommand> command_list;

  // 缓存渲染指令队列
  std::queue<RenderCommand> command_list_temp;

 private:
  // 重置渲染指令队列
  void reset();
  // 确定渲染内容
  void finalize();

 public:
  // 构造RendererManager
  RendererManager(QOpenGLFunctions* glf);
  // 析构RendererManager
  virtual ~RendererManager();

  // 添加矩形
  void addRect(const QRectF& rect, TextureInfo* texture_info,
               const QColor& fill_color, bool is_volatile);
  // 添加椭圆
  void addEllipse(const QRectF& bounds, TextureInfo* texture_info,
                  const QColor& fill_color, bool is_volatile);

  // 渲染全部图形
  void renderAll();
};

#endif  // RENDERER_MANAGER_H
