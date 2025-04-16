#ifndef ABSTRACT_RENDERER_H
#define ABSTRACT_RENDERER_H

#include <cstdint>
#include <memory>
#include <vector>

#include "../texture/pool/BaseTexturePool.h"
#include "RenderCommand.h"
#include "shader/GLShader.h"

class GLCanvas;
class QVector2D;
class QVector4D;
class QMatrix4x4;
enum class TexturePoolType;

enum class InstanceDataType {
  POSITION,
  SIZE,
  ROTATION,
  TEXTURE_POLICY,
  TEXTURE_ID,
  FILL_COLOR,
  RADIUS,
  TEXT,
  GLYPH_ID,
};

class AbstractRenderer {
 public:
  // 构造AbstractRenderer
  AbstractRenderer(GLCanvas* canvas, std::shared_ptr<GLShader> general_shader,
                   int oval_segment, int max_shape_count);
  // 析构AbstractRenderer
  virtual ~AbstractRenderer();

  // 当前正在使用的纹理池
  std::shared_ptr<BaseTexturePool> current_use_pool;

  // 当前shader使用的纹理池类型
  TexturePoolType current_pool_type{TexturePoolType::UNKNOWN};

  // 当前是否使用纹理集
  bool is_current_atlas{false};

  // 子类公共成员
  // gl实例指针
  GLCanvas* cvs;
  // 割圆数
  int oval_segment;
  // 最大图元数
  int max_shape_count;
  // 顶点数组对象
  uint32_t VAO;
  // 顶点缓冲对象
  uint32_t VBO;
  uint32_t EBO;
  // 帧缓冲对象
  uint32_t FBO;
  // 统一缓冲对象
  uint32_t UBO;
  // 着色器
  std::shared_ptr<GLShader> shader;

  // 图形位置数据
  std::vector<QVector2D> position_data;
  // 图形尺寸
  std::vector<QVector2D> size_data;
  // 旋转角度
  std::vector<float> rotation_data;
  // 贴图方式
  std::vector<uint32_t> texture_policy_data;
  // 贴图id
  std::vector<uint32_t> texture_id_data;
  // 填充颜色
  std::vector<QVector4D> fill_color_data;
  // 圆角半径
  std::vector<float> radius_data;

  // 同步数据
  virtual void synchronize_data(InstanceDataType data_type,
                                size_t instance_index, void* data) = 0;
  // 更新gpu数据
  virtual void update_gpu_memory() = 0;

  // 重置更新内容
  virtual void reset_update() = 0;

  // 绑定渲染器
  virtual void bind();

  // 解除绑定渲染器
  virtual void unbind();

  // 渲染指定图形实例
  void render(const ShapeType& shape, uint32_t start_shape_index,
              uint32_t shape_count);
};

#endif  // ABSTRACT_RENDERER_H
