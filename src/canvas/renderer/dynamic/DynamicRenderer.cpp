#include "DynamicRenderer.h"

#include "../../../log/colorful-log.h"

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

DynamicRenderer::DynamicRenderer(QOpenGLFunctions* glfuntions)
    : glf(glfuntions) {}

DynamicRenderer::~DynamicRenderer() {}
// 绑定渲染器
void DynamicRenderer::bind() {}
// 解除绑定渲染器
void DynamicRenderer::unbind() {}

// 渲染向此渲染器提交的全部图形批
void DynamicRenderer::render() {
  GLCALL(glf->glUseProgram(shader_program));
  GLCALL(glf->glUseProgram(0));
}
