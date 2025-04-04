#ifndef ABSTRACT_RENDERER_H
#define ABSTRACT_RENDERER_H

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "RenderCommand.h"

class GLCanvas;
class QVector2D;
class QVector4D;
class QMatrix4x4;
class BaseTexturePool;

enum InstanceDataType {
  POSITION,
  SIZE,
  ROTATION,
  TEXTURE_POLICY,
  TEXTURE_ID,
  FILL_COLOR,
};

class AbstractRenderer {
 protected:
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
  // 着色器程序
  uint32_t shader_program;

  // 当前正在使用的纹理池
  std::shared_ptr<BaseTexturePool> current_texture_pool;

  // uniform位置表缓存
  std::unordered_map<const char*, int32_t> uniform_locations;

  // 图形位置数据
  std::vector<QVector2D> position_data;
  // 图形尺寸
  std::vector<QVector2D> size_data;
  // 旋转角度
  std::vector<float> rotation_data;
  // 贴图方式
  std::vector<int16_t> texture_policy_data;
  // 贴图id
  std::vector<uint32_t> texture_id_data;
  // 填充颜色
  std::vector<QVector4D> fill_color_data;

  // 同步数据
  virtual void synchronize_data(InstanceDataType data_type,
                                size_t instance_index, void* data) = 0;
  // 更新gpu数据
  virtual void update_gpu_memory() = 0;

  virtual void reset_update() = 0;
  friend class GLCanvas;
  friend class RendererManager;

 public:
  // 构造AbstractRenderer
  AbstractRenderer(GLCanvas* canvas, int oval_segment, int max_shape_count);
  // 析构AbstractRenderer
  virtual ~AbstractRenderer();

  // 设置uniform浮点
  void set_uniform_float(const char* location_name, float value);
  // 设置uniform矩阵(4x4)
  void set_uniform_mat4(const char* location_name, const QMatrix4x4& mat);

  // 绑定渲染器
  virtual void bind();
  // 解除绑定渲染器
  virtual void unbind();

  // 渲染指定图形实例
  void render(const ShapeType& shape, uint32_t start_shape_index,
              uint32_t shape_count);
};

#endif  // ABSTRACT_RENDERER_H
