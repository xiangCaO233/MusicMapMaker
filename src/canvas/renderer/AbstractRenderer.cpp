#include "AbstractRenderer.h"

#include "../../log/colorful-log.h"
#include "../GLCanvas.h"
#include "renderer/RenderCommand.h"

// 用于包装 OpenGL 调用并检查错误
#define GLCALL(func)                                       \
  func;                                                    \
  {                                                        \
    XLogger::glcalls++;                                    \
    GLenum error = cvs->glGetError();                      \
    if (error != GL_NO_ERROR) {                            \
      XERROR("在[" + std::string(#func) +                  \
             "]发生OpenGL错误: " + std::to_string(error)); \
    }                                                      \
  }
#define DRAWCALL(func) \
  GLCALL(func);        \
  XLogger::drawcalls++;

AbstractRenderer::AbstractRenderer(GLCanvas* canvas,
                                   std::shared_ptr<Shader> general_shader,
                                   int oval_segment, int max_shape_count)
    : cvs(canvas),
      shader(general_shader),
      oval_segment(oval_segment),
      max_shape_count(max_shape_count) {
  GLCALL(cvs->glGenVertexArrays(1, &VAO));
  GLCALL(cvs->glGenBuffers(1, &VBO));
  GLCALL(cvs->glGenBuffers(1, &EBO));
  GLCALL(cvs->glGenBuffers(1, &FBO));

  // 基本顶点
  std::vector<float> vertices = {
      -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,  // v1 --左下
      1.0f,  -1.0f, 0.0f, 1.0f, 0.0f,  // v2 --右下
      1.0f,  1.0f,  0.0f, 1.0f, 1.0f,  // v3 --右上
      -1.0f, 1.0f,  0.0f, 0.0f, 1.0f,  // v4 --左上
  };

  // 初始化椭圆顶点
  for (int i = 0; i < oval_segment; i++) {
    auto angle = (float(2.0 * M_PI * float(i) / float(oval_segment)));
    float x = cos(angle);
    float y = sin(angle);
    float texcoordx = 0.5f + 0.5f * x;
    float texcoordy = 0.5f + 0.5f * y;
    vertices.push_back(x);
    vertices.push_back(y);
    vertices.push_back(0.0f);
    vertices.push_back(texcoordx);
    vertices.push_back(texcoordy);
  }

  GLCALL(cvs->glBindVertexArray(VAO));
  GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, VBO));

  GLCALL(cvs->glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
                           vertices.data(), GL_STATIC_DRAW));

  // 描述location0 顶点缓冲0~2float为float类型数据(用vec3接收)
  GLCALL(cvs->glEnableVertexAttribArray(0));
  GLCALL(cvs->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                                    nullptr));

  // 描述location1 顶点缓冲3~4float为float类型数据(用vec2接收为默认uv坐标)
  GLCALL(cvs->glEnableVertexAttribArray(1));
  GLCALL(cvs->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                                    (void*)(3 * sizeof(float))));
}

AbstractRenderer::~AbstractRenderer() {
  // 释放顶点数组
  GLCALL(cvs->glDeleteVertexArrays(1, &VAO));
  // 释放缓冲区
  GLCALL(cvs->glDeleteBuffers(1, &VBO));
  GLCALL(cvs->glDeleteBuffers(1, &EBO));
  GLCALL(cvs->glDeleteBuffers(1, &FBO));
};

// 绑定渲染器
void AbstractRenderer::bind() {
  // 绑定顶点数组
  GLCALL(cvs->glBindVertexArray(VAO));
  // 绑定着色器
  shader->use();
}
// 解除绑定渲染器
void AbstractRenderer::unbind() {
  // 取消绑定顶点数组
  GLCALL(cvs->glBindVertexArray(0));
  // 取消绑定着色器
  shader->unuse();
}

// 渲染指定图形实例
void AbstractRenderer::render(const ShapeType& shape,
                              uint32_t start_shape_index,
                              uint32_t shape_count) {
#ifdef __APPLE__
  // TODO(xiang 2025-04-03): 实现apple平台指定实例位置绘制
#else
  switch (shape) {
    case ShapeType::RECT:
    case ShapeType::TEXT: {
      DRAWCALL(cvs->glDrawArraysInstancedBaseInstance(
          GL_TRIANGLE_FAN, 0, 4, shape_count, start_shape_index));
      break;
    }
    case ShapeType::OVAL: {
      DRAWCALL(cvs->glDrawArraysInstancedBaseInstance(
          GL_TRIANGLE_FAN, 4, oval_segment, shape_count, start_shape_index));
      break;
    }
  }
#endif  //__APPLE__
}
