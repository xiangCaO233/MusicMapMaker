#include "DynamicRenderer.h"

#include <QFile>

#include "../../../log/colorful-log.h"
#include "renderer/AbstractRendere.h"

DynamicRenderer::DynamicRenderer(QOpenGLFunctions* glfuntions, int oval_segment)
    : AbstractRenderer(glfuntions, oval_segment) {
  // 初始化实例缓冲区
  GLCALL(glf->glGenBuffers(4, instanceBO));

  // [0] 图形位置,[1] 图形尺寸,[2] 图形贴图uv,[3] 填充颜色
  GLCALL(glf->glBindBuffer(GL_ARRAY_BUFFER, instanceBO[0]));
  // 描述location0 顶点缓冲0~1float为float类型数据--位置信息(用vec2接收)
  GLCALL(glf->glEnableVertexAttribArray(0));
  GLCALL(glf->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float),
                                    nullptr));
  GLCALL(glf->glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW));

  GLCALL(glf->glBindBuffer(GL_ARRAY_BUFFER, instanceBO[1]));
  // 描述location0 顶点缓冲0~1float为float类型数据--尺寸信息(用vec2接收)
  GLCALL(glf->glEnableVertexAttribArray(0));
  GLCALL(glf->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float),
                                    nullptr));
  GLCALL(glf->glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW));

  GLCALL(glf->glBindBuffer(GL_ARRAY_BUFFER, instanceBO[2]));
  // 描述location0 顶点缓冲0~1float为float类型数据--贴图uv信息(用vec2接收)
  GLCALL(glf->glEnableVertexAttribArray(0));
  GLCALL(glf->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float),
                                    nullptr));
  GLCALL(glf->glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW));

  GLCALL(glf->glBindBuffer(GL_ARRAY_BUFFER, instanceBO[3]));
  // 描述location0 顶点缓冲0~3float为float类型数据--填充颜色信息(用vec4接收)
  GLCALL(glf->glEnableVertexAttribArray(0));
  GLCALL(glf->glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                                    nullptr));
  GLCALL(glf->glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW));
}

DynamicRenderer::~DynamicRenderer() {}

// 初始化着色器程序
void DynamicRenderer::init_shader_programe() {
  auto vshader = GLCALL(glf->glCreateShader(GL_VERTEX_SHADER));
  auto fshader = GLCALL(glf->glCreateShader(GL_FRAGMENT_SHADER));

  // 用`:/`前缀访问qrc文件
  QFile vertfile(":/glsl/vertexshader-dynamic.glsl.vert");
  QFile fragfile(":/glsl/fragmentshader-dynamic.glsl.frag");

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
  GLCALL(glf->glShaderSource(vshader, 1, &vertex_shader_source, nullptr));
  GLCALL(glf->glShaderSource(fshader, 1, &fragment_shader_source, nullptr));

  GLCALL(glf->glCompileShader(vshader));
  // 检查编译错误
  int success;
  char infoLog[512];
  GLCALL(glf->glGetShaderiv(vshader, GL_COMPILE_STATUS, &success));
  if (!success) {
    GLCALL(glf->glGetShaderInfoLog(vshader, 512, nullptr, infoLog));
    XCRITICAL("顶点着色器编译出错:\n" + std::string(infoLog));
  } else {
    XINFO("顶点着色器编译成功");
  }

  GLCALL(glf->glCompileShader(fshader));
  // 检查编译错误
  GLCALL(glf->glGetShaderiv(fshader, GL_COMPILE_STATUS, &success));
  if (!success) {
    GLCALL(glf->glGetShaderInfoLog(fshader, 512, nullptr, infoLog));
    XCRITICAL("片段着色器编译出错:\n" + std::string(infoLog));
  } else {
    XINFO("片段着色器编译成功");
  }
  // 链接着色器
  shader_program = GLCALL(glf->glCreateProgram());
  GLCALL(glf->glAttachShader(shader_program, vshader));
  GLCALL(glf->glAttachShader(shader_program, fshader));
  GLCALL(glf->glLinkProgram(shader_program));
  // 检查链接错误
  GLCALL(glf->glGetProgramiv(shader_program, GL_LINK_STATUS, &success));
  if (!success) {
    GLCALL(glf->glGetProgramInfoLog(shader_program, 512, nullptr, infoLog));
    XCRITICAL("链接着色器出错:\n" + std::string(infoLog));
  } else {
    XINFO("着色器程序链接成功");
  }
  // 释放着色器
  GLCALL(glf->glDeleteShader(vshader));
  GLCALL(glf->glDeleteShader(fshader));
}
// 渲染向此渲染器提交的全部图形批
void DynamicRenderer::render() {
  GLCALL(glf->glUseProgram(shader_program));
  GLCALL(glf->glUseProgram(0));
}
