#ifndef DYNAMIC_RENDERER_H
#define DYNAMIC_RENDERER_H

#include "../AbstractRendere.h"

class DynamicRenderer : public AbstractRenderer {
 protected:
  // [0] 图形位置,[1] 图形尺寸,[2] 图形贴图uv,[3] 填充颜色
  uint32_t instanceBO[3];

  // 初始化着色器程序
  void init_shader_programe() override;

 public:
  // 构造GLRenderer
  DynamicRenderer(QOpenGLFunctions* glfuntions, int oval_segment);
  // 析构GLRenderer
  ~DynamicRenderer() override;

  // 绑定渲染器
  void bind();
  // 解除绑定渲染器
  void unbind();

  // 渲染向此渲染器提交的全部图形
  void render() override;
};

#endif  // DYNAMIC_RENDERER_H
