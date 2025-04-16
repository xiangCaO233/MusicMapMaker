#ifndef M_SHADER_H
#define M_SHADER_H

#include <QMatrix4x4>
#include <cstdint>
#include <unordered_map>

class GLCanvas;

class GLShader {
  // gl上下文
  GLCanvas* cvs;

  // 着色器程序
  uint32_t shader_program;

  // uniform位置表缓存
  std::unordered_map<const char*, int32_t> uniform_locations;

  friend class AbstractRenderer;

 public:
  // 构造Shader
  explicit GLShader(GLCanvas* canvas, const char* vertex_source_code_path,
                    const char* fragment_source_code_path);
  // 析构Shader
  virtual ~GLShader();

  // 设置采样器
  void set_sampler(const char* name, int value);
  // 设置uniform浮点
  void set_uniform_float(const char* location_name, float value);
  // 设置uniform整数
  void set_uniform_integer(const char* location_name, int32_t value);
  // 设置uniform矩阵(4x4)
  void set_uniform_mat4(const char* location_name, const QMatrix4x4& mat);

  int32_t uniform_loc(const char* location_name);

  void use();
  void unuse();
};

#endif  // M_SHADER_H
