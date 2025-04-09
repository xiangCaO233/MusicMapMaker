#ifndef STATIC_RENDERER_H
#define STATIC_RENDERER_H

#include <vector>

#include "../AbstractRenderer.h"

class StaticRenderer : public AbstractRenderer {
 protected:
  // 顶点实例缓冲对象
  uint32_t instanceBO;

  // 预计更新gpu内存表(实例索引-实例数)
  std::vector<std::pair<size_t, uint32_t>> update_list;

  // 初始化着色器程序
  void init_shader_programe();

  // 同步数据
  void synchronize_data(InstanceDataType data_type, size_t instance_index,
                        void* data) override;
  // 同步更新位置标记
  void synchronize_update_mark(size_t instance_index);

  // 更新gpu数据
  void update_gpu_memory() override;

  // 重置更新标记
  void reset_update() override;
  friend class RendererManager;

 public:
  // 构造GLRenderer
  StaticRenderer(GLCanvas* canvas, int oval_segment, int max_shape_count);
  // 析构GLRenderer
  ~StaticRenderer() override;
};

#endif  // STATIC_RENDERER_H
