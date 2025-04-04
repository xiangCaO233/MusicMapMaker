#include "AbstractRenderer.h"

#include "../../log/colorful-log.h"
#include "../GLCanvas.h"

// 用于包装 OpenGL 调用并检查错误
#define GLCALL(func)                                       \
  func;                                                    \
  {                                                        \
    GLenum error = cvs->glGetError();                      \
    if (error != GL_NO_ERROR) {                            \
      XERROR("在[" + std::string(#func) +                  \
             "]发生OpenGL错误: " + std::to_string(error)); \
    }                                                      \
  }

AbstractRenderer::AbstractRenderer(GLCanvas* canvas, int oval_segment,
                                   int max_shape_count)
    : cvs(canvas),
      oval_segment(oval_segment),
      max_shape_count(max_shape_count) {
  GLCALL(cvs->glGenVertexArrays(1, &VAO));
  GLCALL(cvs->glGenBuffers(1, &VBO));
  GLCALL(cvs->glGenBuffers(1, &EBO));
  GLCALL(cvs->glGenBuffers(1, &FBO));

  // 基本顶点
  std::vector<float> vertices = {
      -1.0f, -1.0f, 1.0f, 0.0f, 0.0f,  // v1
      1.0f,  -1.0f, 1.0f, 1.0f, 0.0f,  // v2
      1.0f,  1.0f,  1.0f, 1.0f, 1.0f,  // v3
      -1.0f, 1.0f,  1.0f, 0.0f, 1.0f,  // v4
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

AbstractRenderer::~AbstractRenderer() {}

// 绑定渲染器
void AbstractRenderer::bind() {
  // 绑定顶点数组
  GLCALL(cvs->glBindVertexArray(VAO));
  // 绑定着色器
  GLCALL(cvs->glUseProgram(shader_program));
}
// 解除绑定渲染器
void AbstractRenderer::unbind() {
  // 取消绑定顶点数组
  GLCALL(cvs->glBindVertexArray(0));
  // 取消绑定着色器
  GLCALL(cvs->glUseProgram(0));
}

// 设置uniform浮点
void AbstractRenderer::set_uniform_float(const char* location_name,
                                         float value) {
  auto locationit = uniform_locations.find(location_name);
  if (locationit == uniform_locations.end()) {
    // 直接查询
    auto location =
        GLCALL(cvs->glGetUniformLocation(shader_program, location_name));
    locationit = uniform_locations.try_emplace(location_name, location).first;
  }
  // 设置uniform
  GLCALL(cvs->glUniform1f(locationit->second, value));
}

// 设置uniform矩阵(4x4)
void AbstractRenderer::set_uniform_mat4(const char* location_name,
                                        const QMatrix4x4& mat) {
  auto locationit = uniform_locations.find(location_name);
  if (locationit == uniform_locations.end()) {
    // 直接查询
    auto location =
        GLCALL(cvs->glGetUniformLocation(shader_program, location_name));
    locationit = uniform_locations.try_emplace(location_name, location).first;
  }
  // XINFO("更新矩阵->");
  // qDebug() << mat;
  // 设置uniform
  GLCALL(cvs->glUniformMatrix4fv(locationit->second, 1, GL_FALSE, mat.data()));
}
