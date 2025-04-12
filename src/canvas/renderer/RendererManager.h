#ifndef RENDERER_MANAGER_H
#define RENDERER_MANAGER_H

#include <map>
#include <memory>
#include <queue>
#include <string>

#include "RenderCommand.h"
#include "dynamic/DynamicRenderer.h"
#include "static/StaticRenderer.h"

enum class TexturePoolType;
class BaseTexturePool;
class FontRenderer;

// 渲染操作
struct RenderOperation {
  // 此操作包的首绘制指令
  RenderCommand head_command;
  // 此渲染操作的图形类型
  ShapeType shape_type;
  // 此渲染操作的渲染器
  std::shared_ptr<AbstractRenderer> renderer;
  // 此渲染操作使用的纹理池
  std::shared_ptr<BaseTexturePool> texture_pool;
  // 此渲染操作的起始图形索引
  int32_t start_shape_index;
  // 此渲染操作的渲染图形数量
  int32_t render_shape_count;
};

class RendererManager {
 private:
  static uint32_t static_instance_index;
  static uint32_t dynamic_instance_index;
  static uint32_t fontr_instance_index;

  // 纹理池表(纹理池类型-同类型纹理池列表)
  std::map<TexturePoolType, std::vector<std::shared_ptr<BaseTexturePool>>>
      texture_pools;

  // 渲染器
  std::shared_ptr<StaticRenderer> static_renderer;
  std::shared_ptr<DynamicRenderer> dynamic_renderer;
  std::shared_ptr<FontRenderer> font_renderer;

  // 着色器
  std::shared_ptr<Shader> font_shader;
  std::shared_ptr<Shader> general_shader;

  // 渲染指令列表
  std::vector<RenderCommand> command_list;

  // 缓存渲染指令列表
  std::vector<RenderCommand> command_list_temp;

  // 渲染操作队列
  std::queue<RenderOperation> operation_queue;

  // 确定渲染内容
  void finalize();

  // 同步渲染指令到渲染器
  void sync_renderer(const std::shared_ptr<AbstractRenderer>& renderer,
                     RenderCommand& command) const;

  friend class GLCanvas;

 public:
  // 构造RendererManager
  RendererManager(GLCanvas* canvas, int oval_segment,
                  int max_shape_count_per_renderer);
  // 析构RendererManager
  virtual ~RendererManager();

  // 纹理特效
  TextureEffect texture_effect{TextureEffect::NONE};
  // 纹理对齐模式
  TextureAlignMode texture_alignmode{TextureAlignMode::ALIGN_TO_CENTER};
  // 纹理填充模式
  TextureFillMode texture_fillmode{TextureFillMode::SCALLING_AND_TILE};
  // 纹理补充模式
  TextureComplementMode texture_complementmode{
      TextureComplementMode::REPEAT_TEXTURE};

  // 使用指定纹理池
  void use_texture_pool(const std::shared_ptr<BaseTexturePool>& texture_pool);

  // 添加文本
  void addText(const QPointF& pos, std::u8string& text, float font_size,
               std::string font_family, const QColor& fill_color,
               float rotation);

  // 添加矩形
  void addRect(const QRectF& rect, std::shared_ptr<TextureInstace> texture,
               const QColor& fill_color, float rotation, bool is_volatile);

  // 添加圆角矩形
  void addRoundRect(const QRectF& rect, std::shared_ptr<TextureInstace> texture,
                    const QColor& fill_color, float rotation,
                    float radius_ratios, bool is_volatile);

  // 添加椭圆
  void addEllipse(const QRectF& bounds, std::shared_ptr<TextureInstace> texture,
                  const QColor& fill_color, float rotation, bool is_volatile);

  // 设置采样器
  void set_general_sampler(const char* name, int value) const;
  void set_fontpool_sampler(const char* name, int value) const;

  // 设置uniform浮点
  void set_general_uniform_float(const char* location_name, float value) const;
  void set_fontpool_uniform_float(const char* location_name, float value) const;

  // 更新全部投影矩阵
  void update_all_projection_mat(const char* location_name, QMatrix4x4& mat);

  // 设置uniform矩阵(4x4)
  void set_general_uniform_mat4(const char* location_name,
                                QMatrix4x4& mat) const;
  void set_fontpool_uniform_mat4(const char* location_name,
                                 QMatrix4x4& mat) const;

  // 渲染全部图形
  void renderAll();
};

#endif  // RENDERER_MANAGER_H
