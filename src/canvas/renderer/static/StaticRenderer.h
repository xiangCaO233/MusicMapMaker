#ifndef STATIC_RENDERER_H
#define STATIC_RENDERER_H

#include "../AbstractRenderer.h"

class StaticRenderer : public AbstractRenderer {
 protected:
  // 顶点实例缓冲对象
  uint32_t instanceBO;

  // 预计更新gpu内存表(实例索引-实例数)
  std::unordered_map<size_t, uint32_t> update_mapping;

  // 初始化着色器程序
  void init_shader_programe();

  // 同步数据
  void synchronize_data(InstanceDataType data_type, size_t instance_index,
                        void* data) override;
  // 同步更新位置标记
  void synchronize_update_mark(size_t instance_index);

  // 更新gpu数据
  void update_gpu_memory() override;
  friend class RendererManager;

 public:
  // 构造GLRenderer
  StaticRenderer(GLCanvas* canvas, int oval_segment, int max_shape_count);
  // 析构GLRenderer
  ~StaticRenderer() override;

  // 渲染指定图形实例
  void render(const ShapeType& shape, uint32_t start_shape_index,
              uint32_t count) override;
};

#endif  // STATIC_RENDERER_H
