#include "DynamicRenderer.h"

#include <QFile>

#include "../../../log/colorful-log.h"
#include "../../GLCanvas.h"

DynamicRenderer::DynamicRenderer(GLCanvas* canvas, int oval_segment,
                                 int max_shape_count)
    : AbstractRenderer(canvas, oval_segment, max_shape_count) {
  // 初始化实例缓冲区
  GLCALL(cvs->glGenBuffers(6, instanceBO));

  // [0] 图形位置,[1] 图形尺寸,[2] 旋转角度
  // [3] 图形贴图方式,[4] 贴图id,[5]填充颜色

  // 位置信息
  GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, instanceBO[0]));
  // 描述location2 顶点缓冲0~1float为float类型数据--位置信息(用vec2接收)
  GLCALL(cvs->glEnableVertexAttribArray(2));
  GLCALL(cvs->glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float),
                                    nullptr));
  GLCALL(cvs->glBufferData(GL_ARRAY_BUFFER,
                           (int)(max_shape_count * 2 * sizeof(float)), nullptr,
                           GL_DYNAMIC_DRAW));
  // 每个实例变化一次
  GLCALL(cvs->glVertexAttribDivisor(2, 1));

  // 尺寸信息
  GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, instanceBO[1]));
  // 描述location3 顶点缓冲0~1float为float类型数据--尺寸信息(用vec2接收)
  GLCALL(cvs->glEnableVertexAttribArray(3));
  GLCALL(cvs->glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float),
                                    nullptr));
  GLCALL(cvs->glBufferData(GL_ARRAY_BUFFER,
                           (int)(max_shape_count * 2 * sizeof(float)), nullptr,
                           GL_DYNAMIC_DRAW));
  GLCALL(cvs->glVertexAttribDivisor(3, 1));

  // 旋转角度信息
  GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, instanceBO[2]));
  // 描述location4 顶点缓冲0~0float为float类型数据--旋转角度信息(用float接收)
  GLCALL(cvs->glEnableVertexAttribArray(4));
  GLCALL(cvs->glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, 1 * sizeof(float),
                                    nullptr));
  GLCALL(cvs->glBufferData(GL_ARRAY_BUFFER,
                           (int)(max_shape_count * 1 * sizeof(float)), nullptr,
                           GL_DYNAMIC_DRAW));
  GLCALL(cvs->glVertexAttribDivisor(4, 1));

  // 贴图uv方式
  GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, instanceBO[3]));
  // 描述location5 顶点缓冲0~0float为float类型数据--贴图uv方式(用float接收)
  GLCALL(cvs->glEnableVertexAttribArray(5));
  GLCALL(cvs->glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, 2 * sizeof(float),
                                    nullptr));
  GLCALL(cvs->glBufferData(GL_ARRAY_BUFFER,
                           (int)(max_shape_count * 1 * sizeof(float)), nullptr,
                           GL_DYNAMIC_DRAW));
  GLCALL(cvs->glVertexAttribDivisor(5, 1));

  // 贴图id信息
  GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, instanceBO[4]));
  // 描述location6 顶点缓冲0~0float为float类型数据--贴图id信息(用float接收)
  GLCALL(cvs->glEnableVertexAttribArray(6));
  GLCALL(cvs->glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, 1 * sizeof(float),
                                    nullptr));
  GLCALL(cvs->glBufferData(GL_ARRAY_BUFFER,
                           (int)(max_shape_count * 1 * sizeof(float)), nullptr,
                           GL_DYNAMIC_DRAW));
  GLCALL(cvs->glVertexAttribDivisor(6, 1));

  // 填充颜色信息
  GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, instanceBO[3]));
  // 描述location7 顶点缓冲0~3float为float类型数据--填充颜色信息(用vec4接收)
  GLCALL(cvs->glEnableVertexAttribArray(7));
  GLCALL(cvs->glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                                    nullptr));
  GLCALL(cvs->glBufferData(GL_ARRAY_BUFFER,
                           (int)(max_shape_count * 4 * sizeof(float)), nullptr,
                           GL_DYNAMIC_DRAW));
  GLCALL(cvs->glVertexAttribDivisor(7, 1));
}

DynamicRenderer::~DynamicRenderer() {}

// 初始化着色器程序
void DynamicRenderer::init_shader_programe() {
  auto vshader = GLCALL(cvs->glCreateShader(GL_VERTEX_SHADER));
  auto fshader = GLCALL(cvs->glCreateShader(GL_FRAGMENT_SHADER));

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
  GLCALL(cvs->glShaderSource(vshader, 1, &vertex_shader_source, nullptr));
  GLCALL(cvs->glShaderSource(fshader, 1, &fragment_shader_source, nullptr));

  GLCALL(cvs->glCompileShader(vshader));
  // 检查编译错误
  int success;
  char infoLog[512];
  GLCALL(cvs->glGetShaderiv(vshader, GL_COMPILE_STATUS, &success));
  if (!success) {
    GLCALL(cvs->glGetShaderInfoLog(vshader, 512, nullptr, infoLog));
    XCRITICAL("顶点着色器编译出错:\n" + std::string(infoLog));
  } else {
    XINFO("顶点着色器编译成功");
  }

  GLCALL(cvs->glCompileShader(fshader));
  // 检查编译错误
  GLCALL(cvs->glGetShaderiv(fshader, GL_COMPILE_STATUS, &success));
  if (!success) {
    GLCALL(cvs->glGetShaderInfoLog(fshader, 512, nullptr, infoLog));
    XCRITICAL("片段着色器编译出错:\n" + std::string(infoLog));
  } else {
    XINFO("片段着色器编译成功");
  }
  // 链接着色器
  shader_program = GLCALL(cvs->glCreateProgram());
  GLCALL(cvs->glAttachShader(shader_program, vshader));
  GLCALL(cvs->glAttachShader(shader_program, fshader));
  GLCALL(cvs->glLinkProgram(shader_program));
  // 检查链接错误
  GLCALL(cvs->glGetProgramiv(shader_program, GL_LINK_STATUS, &success));
  if (!success) {
    GLCALL(cvs->glGetProgramInfoLog(shader_program, 512, nullptr, infoLog));
    XCRITICAL("链接着色器出错:\n" + std::string(infoLog));
  } else {
    XINFO("着色器程序链接成功");
  }
  // 释放着色器
  GLCALL(cvs->glDeleteShader(vshader));
  GLCALL(cvs->glDeleteShader(fshader));
}
// 渲染向此渲染器提交的全部图形批
void DynamicRenderer::render() {
  GLCALL(cvs->glUseProgram(shader_program));
  GLCALL(cvs->glUseProgram(0));
}
