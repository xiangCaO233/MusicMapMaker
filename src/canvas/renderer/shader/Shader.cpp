#include "Shader.h"

#include <QFile>

#include "../../../log/colorful-log.h"
#include "../../GLCanvas.h"

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

Shader::Shader(GLCanvas* canvas, const char* vertex_source_code_path,
               const char* fragment_source_code_path)
    : cvs(canvas) {
  auto vshader = GLCALL(cvs->glCreateShader(GL_VERTEX_SHADER));
  auto fshader = GLCALL(cvs->glCreateShader(GL_FRAGMENT_SHADER));

  QFile vertfile(vertex_source_code_path);
  QFile fragfile(fragment_source_code_path);

  // 检查文件是否成功打开
  if (!vertfile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    auto errormsg = vertfile.errorString();
    auto errorstr = errormsg.toStdString();
    XERROR("Failed to open vertex source file:" + errorstr);
  }
  if (!fragfile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    auto errormsg = fragfile.errorString();
    auto errorstr = errormsg.toStdString();
    XERROR("Failed to open vertex source file:" + errorstr);
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
    XCRITICAL(std::string(vertex_source_code_path) + "顶点着色器编译出错:\n" +
              std::string(infoLog));
  } else {
    XINFO("顶点着色器编译成功");
  }

  GLCALL(cvs->glCompileShader(fshader));
  // 检查编译错误
  GLCALL(cvs->glGetShaderiv(fshader, GL_COMPILE_STATUS, &success));
  if (!success) {
    GLCALL(cvs->glGetShaderInfoLog(fshader, 512, nullptr, infoLog));
    XCRITICAL(std::string(fragment_source_code_path) + "片段着色器编译出错:\n" +
              std::string(infoLog));
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

Shader::~Shader() { GLCALL(cvs->glDeleteProgram(shader_program)); }

void Shader::use() {
  // 绑定着色器
  GLCALL(cvs->glUseProgram(shader_program));
}
void Shader::unuse() {
  // 取消绑定着色器
  GLCALL(cvs->glUseProgram(0));
}

// 设置采样器
void Shader::set_sampler(const char* name, int value) {
  auto location = GLCALL(cvs->glGetUniformLocation(shader_program, name));
  GLCALL(cvs->glUniform1i(location, value));
}

// 设置uniform浮点
void Shader::set_uniform_float(const char* location_name, float value) {
  // 设置uniform
  GLCALL(cvs->glUniform1f(uniform_loc(location_name), value));
}

// 设置uniform整数
void Shader::set_uniform_integer(const char* location_name, int32_t value) {
  // 设置uniform
  GLCALL(cvs->glUniform1i(uniform_loc(location_name), value));
}

// 设置uniform矩阵(4x4)
void Shader::set_uniform_mat4(const char* location_name,
                              const QMatrix4x4& mat) {
  // 设置uniform
  GLCALL(cvs->glUniformMatrix4fv(uniform_loc(location_name), 1, GL_FALSE,
                                 mat.data()));
}

int32_t Shader::uniform_loc(const char* location_name) {
  auto locationit = uniform_locations.find(location_name);
  if (locationit == uniform_locations.end()) {
    // 直接查询
    auto location =
        GLCALL(cvs->glGetUniformLocation(shader_program, location_name));
    locationit = uniform_locations.try_emplace(location_name, location).first;
  }
  return locationit->second;
}
