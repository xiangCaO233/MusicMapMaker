#include "StaticRenderer.h"

#include <QFile>

#include "../../../log/colorful-log.h"
#include "renderer/AbstractRendere.h"

StaticRenderer::StaticRenderer(QOpenGLFunctions* glfuntions, int oval_segment)
    : AbstractRenderer(glfuntions, oval_segment) {
  // 初始化实例缓冲区
  glf->glGenBuffers(1, &instanceBO);
  // 图形位置2f,图形尺寸2f,图形贴图uv2f,填充颜色4f
  GLCALL(glf->glBindBuffer(GL_ARRAY_BUFFER, instanceBO));

  // 描述location0 顶点缓冲0~1float为float类型数据--位置信息(用vec2接收)
  GLCALL(glf->glEnableVertexAttribArray(0));
  GLCALL(glf->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                                    10 * sizeof(float), nullptr));
  // 描述location1 顶点缓冲2~3float为float类型数据--尺寸信息(用vec2接收)
  GLCALL(glf->glEnableVertexAttribArray(0));
  GLCALL(glf->glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 2 * sizeof(float),
                                    nullptr));
  // 描述location2 顶点缓冲4~5float为float类型数据--贴图uv信息(用vec2接收)
  GLCALL(glf->glEnableVertexAttribArray(0));
  GLCALL(glf->glVertexAttribPointer(4, 5, GL_FLOAT, GL_FALSE, 2 * sizeof(float),
                                    nullptr));
  // 描述location3 顶点缓冲6~9float为float类型数据--填充颜色信息(用vec4接收)
  GLCALL(glf->glEnableVertexAttribArray(0));
  GLCALL(glf->glVertexAttribPointer(6, 9, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                                    nullptr));

  GLCALL(glf->glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW));
}

StaticRenderer::~StaticRenderer() {}
// 初始化着色器程序
void StaticRenderer::init_shader_programe() {
  auto vshader = GLCALL(glf->glCreateShader(GL_VERTEX_SHADER));
  auto fshader = GLCALL(glf->glCreateShader(GL_FRAGMENT_SHADER));

  // 用`:/`前缀访问qrc文件
  QFile vertfile(":/glsl/vertexshader-static.glsl.vert");
  QFile fragfile(":/glsl/fragmentshader-static.glsl.frag");

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
void StaticRenderer::render() {
  GLCALL(glf->glUseProgram(shader_program));
  GLCALL(glf->glUseProgram(0));
}
