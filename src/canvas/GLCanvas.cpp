#include "GLCanvas.h"

#include <QFile>
#include <QTextStream>

#include "colorful-log.h"

// 用于包装 OpenGL 调用并检查错误
#define GLCALL(func)                                       \
  func;                                                    \
  {                                                        \
    GLenum error = glGetError();                           \
    if (error != GL_NO_ERROR) {                            \
      XERROR("在[" + std::string(#func) +                  \
             "]发生OpenGL错误: " + std::to_string(error)); \
    }                                                      \
  }

GLCanvas::GLCanvas(QWidget *parent) : QOpenGLWidget(parent) {}

GLCanvas::~GLCanvas() = default;

void GLCanvas::initializeGL() {
  initializeOpenGLFunctions();
  initbuffer();
  initshader();
}
void GLCanvas::resizeGL(int w, int h) { glViewport(0, 0, w, h); }

// 绘制画布
void GLCanvas::paintGL() {
  GLCALL(glUseProgram(shader_program));
  GLCALL(glClearColor(0.23f, 0.23f, 0.23f, 1.0f));
  GLCALL(glClear(GL_COLOR_BUFFER_BIT));
  GLCALL(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));
  GLCALL(glUseProgram(0));
}

// 初始化缓冲区
void GLCanvas::initbuffer() {
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &instanceBO);
  glGenBuffers(1, &EBO);
  glGenBuffers(1, &FBO);

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

  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);

  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
               vertices.data(), GL_STATIC_DRAW);

  // 描述location0 顶点缓冲0~2float为float类型数据(用vec3接收)
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);

  // 描述location1 顶点缓冲3~4float为float类型数据(用vec2接收为默认uv坐标)
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                        (void *)(3 * sizeof(float)));
}

// 初始化着色器程序
void GLCanvas::initshader() {
  auto vshader = GLCALL(glCreateShader(GL_VERTEX_SHADER));
  auto fshader = GLCALL(glCreateShader(GL_FRAGMENT_SHADER));

  // 用`:/`前缀访问qrc文件
  QFile vertfile(":/glsl/vertexshader.glsl.vert");
  QFile fragfile(":/glsl/fragmentshader.glsl.frag");

  // 检查文件是否成功打开
  if (!vertfile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qDebug() << "Failed to open vertex source file:" << vertfile.errorString();
  }
  if (!fragfile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qDebug() << "Failed to open vertex source file:" << fragfile.errorString();
  }

  // 用QTextStream读取内容
  QTextStream vertin(&vertfile);
  QTextStream fragin(&fragfile);

  auto vertex_shader_qstr = vertin.readAll();
  auto fragment_shader_qstr = fragin.readAll();

  auto vertex_shader_str = vertex_shader_qstr.toStdString();
  auto fragment_shader_str = fragment_shader_qstr.toStdString();

  auto vertex_shader_source = vertex_shader_str.c_str();
  auto fragment_shader_source = fragment_shader_str.c_str();

  // 关闭文件
  vertfile.close();
  fragfile.close();

  // 注入源代码
  GLCALL(glShaderSource(vshader, 1, &vertex_shader_source, nullptr));
  GLCALL(glShaderSource(fshader, 1, &fragment_shader_source, nullptr));

  GLCALL(glCompileShader(vshader));
  // 检查编译错误
  int success;
  char infoLog[512];
  GLCALL(glGetShaderiv(vshader, GL_COMPILE_STATUS, &success));
  if (!success) {
    GLCALL(glGetShaderInfoLog(vshader, 512, nullptr, infoLog));
    XCRITICAL("顶点着色器编译出错:\n" + std::string(infoLog));
  } else {
    XINFO("顶点着色器编译成功");
  }

  GLCALL(glCompileShader(fshader));
  // 检查编译错误
  GLCALL(glGetShaderiv(fshader, GL_COMPILE_STATUS, &success));
  if (!success) {
    GLCALL(glGetShaderInfoLog(fshader, 512, nullptr, infoLog));
    XCRITICAL("片段着色器编译出错:\n" + std::string(infoLog));
  } else {
    XINFO("片段着色器编译成功");
  }
  // 链接着色器
  shader_program = glCreateProgram();
  GLCALL(glAttachShader(shader_program, vshader));
  GLCALL(glAttachShader(shader_program, fshader));
  GLCALL(glLinkProgram(shader_program));
  // 检查链接错误
  GLCALL(glGetProgramiv(shader_program, GL_LINK_STATUS, &success));
  if (!success) {
    GLCALL(glGetProgramInfoLog(shader_program, 512, nullptr, infoLog));
    XCRITICAL("链接着色器出错:\n" + std::string(infoLog));
  } else {
    XINFO("着色器程序链接成功");
  }
  // 释放着色器
  GLCALL(glDeleteShader(vshader));
  GLCALL(glDeleteShader(fshader));
}
