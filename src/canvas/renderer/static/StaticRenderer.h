#ifndef STATIC_RENDERER_H
#define STATIC_RENDERER_H

#include "../AbstractRenderer.h"

class StaticRenderer : public AbstractRenderer {
 protected:
  // 顶点实例缓冲对象
  uint32_t instanceBO;
  // 初始化着色器程序
  void init_shader_programe() override;

 public:
  // 构造GLRenderer
  StaticRenderer(GLCanvas* canvas, int oval_segment, int max_shape_count);
  // 析构GLRenderer
  ~StaticRenderer() override;

  // 渲染向此渲染器提交的全部图形批
  void render() override;
};

#endif  // STATIC_RENDERER_H
