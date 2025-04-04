#ifndef RENDERER_MANAGER_H
#define RENDERER_MANAGER_H

#include <map>
#include <memory>
#include <queue>

#include "RenderCommand.h"
#include "dynamic/DynamicRenderer.h"
#include "static/StaticRenderer.h"

enum class TexturePoolType;
class BaseTexturePool;

// 渲染操作
struct RenderOperation {
  // 此渲染操作的图形类型
  ShapeType shape_type;
  // 此渲染操作的渲染器
  std::shared_ptr<AbstractRenderer> renderer;
  // 此渲染操作的起始图形索引
  int32_t start_shape_index;
  // 此渲染操作的渲染图形数量
  int32_t render_shape_count;
};

class RendererManager {
 private:
  static uint32_t static_instance_index;
  static uint32_t dynamic_instance_index;

  // 纹理池表(纹理池类型-同类型纹理池列表)
  std::map<TexturePoolType, std::vector<std::shared_ptr<BaseTexturePool>>>
      texture_pools;

  // 是否处理好准备渲染
  bool is_ready{false};

  // 渲染器
  std::shared_ptr<StaticRenderer> static_renderer;
  std::shared_ptr<DynamicRenderer> dynamic_renderer;

  // 渲染指令列表
  std::vector<RenderCommand> command_list;

  // 缓存渲染指令列表
  std::vector<RenderCommand> command_list_temp;

  // 渲染操作队列
  std::queue<RenderOperation> operation_queue;

  // 确定渲染内容
  void finalize();

  friend class GLCanvas;

 public:
  // 构造RendererManager
  RendererManager(GLCanvas* canvas, int oval_segment,
                  int max_shape_count_per_renderer);
  // 析构RendererManager
  virtual ~RendererManager();

  // 添加矩形
  void addRect(const QRectF& rect, TextureInfo* texture_info,
               const QColor& fill_color, float rotation, bool is_volatile);
  // 添加椭圆
  void addEllipse(const QRectF& bounds, TextureInfo* texture_info,
                  const QColor& fill_color, float rotation, bool is_volatile);

  // 设置uniform浮点
  void set_uniform_float(const char* location_name, float value);
  // 设置uniform矩阵(4x4)
  void set_uniform_mat4(const char* location_name, QMatrix4x4& mat);

  // 渲染全部图形
  void renderAll();
};

#endif  // RENDERER_MANAGER_H
