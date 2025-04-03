#ifndef DYNAMIC_RENDERER_H
#define DYNAMIC_RENDERER_H

#include <QOpenGLFunctions>
#include <cstdint>
#include <vector>

#include "../RenderCommand.h"

class DynamicRenderer {
  // gl函数
  QOpenGLFunctions* glf;
  // 顶点数组对象
  uint32_t VAO;
  // 顶点缓冲对象
  uint32_t VBO;
  uint32_t instanceBO;
  uint32_t EBO;
  // 帧缓冲对象
  uint32_t FBO;
  // 统一缓冲对象
  uint32_t UBO;
  // 着色器程序
  uint32_t shader_program;

  // 渲染批列表
  std::vector<DrawBatch*> batches;

 public:
  // 构造GLRenderer
  DynamicRenderer(QOpenGLFunctions* glfuntions);
  // 析构GLRenderer
  virtual ~DynamicRenderer();

  // 绑定渲染器
  void bind();
  // 解除绑定渲染器
  void unbind();

  // 渲染向此渲染器提交的全部图形
  void render();
};

#endif  // DYNAMIC_RENDERER_H
