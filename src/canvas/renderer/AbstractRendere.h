#ifndef ABSTRACT_RENDERER_H
#define ABSTRACT_RENDERER_H

#include <QOpenGLFunctions_4_1_Core>
#include <cstdint>
#include <vector>

#include "RenderCommand.h"

// 用于包装 OpenGL 调用并检查错误
#define GLCALL(func)                                       \
  func;                                                    \
  {                                                        \
    GLenum error = glf->glGetError();                      \
    if (error != GL_NO_ERROR) {                            \
      XERROR("在[" + std::string(#func) +                  \
             "]发生OpenGL错误: " + std::to_string(error)); \
    }                                                      \
  }

class AbstractRenderer {
 protected:
  // 子类公共成员
  // gl函数
  QOpenGLFunctions_4_1_Core* glf;
  // 割圆数
  int oval_segment;
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

  // 渲染批列表
  std::vector<DrawBatch*> batches;

  // 初始化着色器程序
  virtual void init_shader_programe();

 public:
  // 构造AbstractRenderer
  AbstractRenderer(QOpenGLFunctions* glfuntions, int oval_segment);
  // 析构AbstractRenderer
  virtual ~AbstractRenderer();

  // 绑定渲染器
  virtual void bind();
  // 解除绑定渲染器
  virtual void unbind();

  // 渲染向此渲染器提交的全部图形批
  virtual void render();
};

#endif  // ABSTRACT_RENDERER_H
