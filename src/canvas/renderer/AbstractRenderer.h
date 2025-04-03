#ifndef ABSTRACT_RENDERER_H
#define ABSTRACT_RENDERER_H

#include <QOpenGLFunctions_4_1_Core>
#include <cstdint>
#include <vector>

#include "RenderCommand.h"

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

class GLCanvas;

class AbstractRenderer {
 protected:
  // 子类公共成员
  // gl实例指针
  GLCanvas* cvs;
  // 割圆数
  int oval_segment;
  // 最大图元数
  int max_shape_count;
  // 顶点数组对象
  uint32_t VAO;
  // 顶点缓冲对象
  uint32_t VBO;
  uint32_t EBO;
  // 帧缓冲对象
  uint32_t FBO;
  // 统一缓冲对象
  uint32_t UBO;
  // 着色器程序
  uint32_t shader_program;

  // uniform位置表缓存
  std::unordered_map<const char*, int32_t> uniform_locations;

  // 图形位置数据
  std::vector<QVector2D> position_data;
  // 图形尺寸
  std::vector<QVector2D> size_data;
  // 旋转角度
  std::vector<float> rotation_data;
  // 图形贴图方式
  struct TexturePolicy {
    TextureAlignMode align_mode;
    TextureFillMode fill_mode;
  };
  std::vector<TexturePolicy> texture_policy_data;
  // 贴图id
  std::vector<float> texture_id_data;
  // 填充颜色
  std::vector<QVector4D> fill_color_data;

  // 渲染批列表
  std::vector<DrawBatch*> batches;

  friend class GLCanvas;

 public:
  // 构造AbstractRenderer
  AbstractRenderer(GLCanvas* canvas, int oval_segment, int max_shape_count);
  // 析构AbstractRenderer
  virtual ~AbstractRenderer();

  // 设置uniform浮点
  void set_uniform_float(const char* location_name, float value);
  // 设置uniform矩阵(4x4)
  void set_uniform_mat4(const char* location_name, QMatrix4x4& mat);

  // 绑定渲染器
  virtual void bind();
  // 解除绑定渲染器
  virtual void unbind();

  // 渲染向此渲染器提交的全部图形批
  virtual void render(uint32_t start_shape_index, uint32_t count) = 0;
};

#endif  // ABSTRACT_RENDERER_H
