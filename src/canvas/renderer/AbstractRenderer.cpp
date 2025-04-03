#include "AbstractRenderer.h"

#include "../GLCanvas.h"

AbstractRenderer::AbstractRenderer(GLCanvas* canvas, int oval_segment,
                                   int max_shape_count)
    : cvs(canvas),
      oval_segment(oval_segment),
      max_shape_count(max_shape_count) {
  cvs->glGenVertexArrays(1, &VAO);
  cvs->glGenBuffers(1, &VBO);
  cvs->glGenBuffers(1, &EBO);
  cvs->glGenBuffers(1, &FBO);

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

  cvs->glBindVertexArray(VAO);
  cvs->glBindBuffer(GL_ARRAY_BUFFER, VBO);

  cvs->glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
                    vertices.data(), GL_STATIC_DRAW);

  // 描述location0 顶点缓冲0~2float为float类型数据(用vec3接收)
  cvs->glEnableVertexAttribArray(0);
  cvs->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                             nullptr);

  // 描述location1 顶点缓冲3~4float为float类型数据(用vec2接收为默认uv坐标)
  cvs->glEnableVertexAttribArray(1);
  cvs->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                             (void*)(3 * sizeof(float)));

  // 初始化着色器程序
  init_shader_programe();
}

AbstractRenderer::~AbstractRenderer() {}

void AbstractRenderer::init_shader_programe() {}

// 绑定渲染器
void AbstractRenderer::bind() {
  // 绑定顶点数组
  cvs->glBindVertexArray(VAO);
}
// 解除绑定渲染器
void AbstractRenderer::unbind() {
  // 取消绑定顶点数组
  cvs->glBindVertexArray(0);
}
