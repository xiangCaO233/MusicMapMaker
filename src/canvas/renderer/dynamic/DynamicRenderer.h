#ifndef DYNAMIC_RENDERER_H
#define DYNAMIC_RENDERER_H

#include "../AbstractRenderer.h"

class DynamicRenderer : public AbstractRenderer {
 protected:
  // [0] 图形位置,[1] 图形尺寸,[2] 旋转角度
  // [3] 图形贴图方式,[4] 贴图id,[5]填充颜色
  uint32_t instanceBO[5];

  // 初始化着色器程序
  void init_shader_programe();

  // 同步数据
  void synchronize_data(InstanceDataType data_type, size_t instance_index,
                        void* data) override;
  // 更新gpu数据
  virtual void update_gpu_memory() override;

  friend class RendererManager;

 public:
  // 构造GLRenderer
  DynamicRenderer(GLCanvas* canvas, int oval_segment, int max_shape_count);
  // 析构GLRenderer
  ~DynamicRenderer() override;

  // 渲染指定图形实例
  void render(const ShapeType& shape, uint32_t start_shape_index,
              uint32_t count) override;
};

#endif  // DYNAMIC_RENDERER_H
