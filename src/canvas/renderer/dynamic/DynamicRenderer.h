#ifndef DYNAMIC_RENDERER_H
#define DYNAMIC_RENDERER_H

#include <map>
#include <vector>

#include "../AbstractRenderer.h"

class DynamicRenderer : public AbstractRenderer {
 protected:
  // [0] 图形位置,[1] 图形尺寸,[2] 旋转角度
  // [3] 图形贴图方式,[4] 贴图id,[5]填充颜色
  uint32_t instanceBO[6];

  // 预计更新gpu内存表(缓冲区id-(实例索引-实例数))
  std::unordered_map<InstanceDataType, std::vector<std::pair<size_t, uint32_t>>>
      update_mapping;

  // 初始化着色器程序
  void init_shader_programe();

  // 同步更新标记
  void synchronize_update_mark(InstanceDataType data_type,
                               size_t instance_index);

  // 同步数据
  void synchronize_data(InstanceDataType data_type, size_t instance_index,
                        void* data) override;
  // 更新gpu数据
  virtual void update_gpu_memory() override;

  // 重置更新标记
  void reset_update() override;

  friend class RendererManager;

 public:
  // 构造GLRenderer
  DynamicRenderer(GLCanvas* canvas, int oval_segment, int max_shape_count);
  // 析构GLRenderer
  ~DynamicRenderer() override;
};

#endif  // DYNAMIC_RENDERER_H
