#include "StaticRenderer.h"

#include <QFile>
#include <vector>

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

StaticRenderer::StaticRenderer(GLCanvas* canvas, int oval_segment,
                               int max_shape_count)
    : AbstractRenderer(canvas, oval_segment, max_shape_count) {
  // 初始化实例缓冲区
  GLCALL(cvs->glGenBuffers(1, &instanceBO));
  // 图形位置2f,图形尺寸2f,旋转角度1f,图形贴图uv2f,贴图id1f,填充颜色4f
  GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, instanceBO));

  // 位置信息
  // 描述location2 顶点缓冲0~1float为float类型数据--位置信息(用vec2接收)
  GLCALL(cvs->glEnableVertexAttribArray(2));
  GLCALL(cvs->glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
                                    11 * sizeof(float), nullptr));
  // 每个实例变化一次
  GLCALL(cvs->glVertexAttribDivisor(2, 1));

  // 尺寸信息
  // 描述location3 顶点缓冲2~3float为float类型数据--尺寸信息(用vec2接收)
  GLCALL(cvs->glEnableVertexAttribArray(3));
  GLCALL(cvs->glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE,
                                    11 * sizeof(float),
                                    (void*)(2 * sizeof(float))));
  GLCALL(cvs->glVertexAttribDivisor(3, 1));

  // 旋转角度
  // 描述location4 顶点缓冲4~4float为float类型数据--旋转角度(用float接收)
  GLCALL(cvs->glEnableVertexAttribArray(4));
  GLCALL(cvs->glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE,
                                    11 * sizeof(float),
                                    (void*)(4 * sizeof(float))));
  GLCALL(cvs->glVertexAttribDivisor(4, 1));

  // 贴图uv方式
  // 描述location5 顶点缓冲5~5float为float类型数据--贴图uv方式(用float接收)
  GLCALL(cvs->glEnableVertexAttribArray(5));
  GLCALL(cvs->glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE,
                                    11 * sizeof(float),
                                    (void*)(5 * sizeof(float))));
  GLCALL(cvs->glVertexAttribDivisor(5, 1));

  // 贴图id信息
  // 描述location6 顶点缓冲6~6float为float类型数据--贴图id信息(用float接收)
  GLCALL(cvs->glEnableVertexAttribArray(6));
  GLCALL(cvs->glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE,
                                    11 * sizeof(float),
                                    (void*)(6 * sizeof(float))));
  GLCALL(cvs->glVertexAttribDivisor(6, 1));

  // 填充颜色信息
  // 描述location7 顶点缓冲7~10float为float类型数据--填充颜色信息(用vec4接收)
  GLCALL(cvs->glEnableVertexAttribArray(7));
  GLCALL(cvs->glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE,
                                    11 * sizeof(float),
                                    (void*)(7 * sizeof(float))));
  GLCALL(cvs->glVertexAttribDivisor(7, 1));

  GLCALL(cvs->glBufferData(GL_ARRAY_BUFFER,
                           (int)(max_shape_count * 11 * sizeof(float)), nullptr,
                           GL_STATIC_DRAW));

  // 初始化着色器程序
  init_shader_programe();
}

StaticRenderer::~StaticRenderer() {}

// 初始化着色器程序
void StaticRenderer::init_shader_programe() {
  auto vshader = GLCALL(cvs->glCreateShader(GL_VERTEX_SHADER));
  auto fshader = GLCALL(cvs->glCreateShader(GL_FRAGMENT_SHADER));
  // 用`:/`前缀访问qrc文件
#ifdef __APPLE__
  QFile vertfile(":/glsl/macos/vertexshader-static.glsl.vert");
  QFile fragfile(":/glsl/macos/fragmentshader-static.glsl.frag");
#else
  QFile vertfile(":/glsl/vertexshader-static.glsl.vert");
  QFile fragfile(":/glsl/fragmentshader-static.glsl.frag");
#endif  //__APPLE__

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

// 同步数据
void StaticRenderer::synchronize_data(InstanceDataType data_type,
                                      size_t instance_index, void* data) {
  switch (data_type) {
    case POSITION: {
      auto pos = static_cast<QVector2D*>(data);
      if (position_data.empty() || position_data.size() <= instance_index) {
        // XWARN("添加位置数据");
        position_data.push_back(*pos);
        synchronize_update_mark(instance_index);
      } else {
        if (*pos != position_data.at(instance_index)) {
          // 位置数据发生变化
          // 同步更新位置标记
          synchronize_update_mark(instance_index);
          // 更新此实例位置数据
          position_data[instance_index] = *pos;
        }
      }
      break;
    }
    case SIZE: {
      auto size = static_cast<QVector2D*>(data);
      if (size_data.empty() || size_data.size() <= instance_index) {
        // XWARN("添加尺寸数据");
        size_data.push_back(*size);
        synchronize_update_mark(instance_index);
      } else {
        if (*size != size_data.at(instance_index)) {
          // 尺寸数据发生变化
          // 同步更新位置标记
          synchronize_update_mark(instance_index);
          // 更新此实例尺寸数据
          size_data[instance_index] = *size;
        }
      }
      break;
    }
    case ROTATION: {
      auto rotation = static_cast<float*>(data);
      if (rotation_data.empty() || rotation_data.size() <= instance_index) {
        // XWARN("添加角度数据");
        rotation_data.push_back(*rotation);
        synchronize_update_mark(instance_index);
      } else {
        if (*rotation != rotation_data.at(instance_index)) {
          // 角度数据发生变化
          // 同步更新位置标记
          synchronize_update_mark(instance_index);
          // 更新此实例角度数据
          rotation_data[instance_index] = *rotation;
        }
      }
      break;
    }
    case TEXTURE_POLICY: {
      auto texture_policy = static_cast<int32_t*>(data);
      if (texture_policy_data.empty() ||
          texture_policy_data.size() <= instance_index) {
        // XWARN("添加纹理填充策略数据");
        texture_policy_data.push_back(*texture_policy);
        synchronize_update_mark(instance_index);
      } else {
        if (*texture_policy != texture_policy_data.at(instance_index)) {
          // 贴图策略数据发生变化
          // 同步更新位置标记
          synchronize_update_mark(instance_index);
          // 更新此实例贴图策略数据
          texture_policy_data[instance_index] = *texture_policy;
        }
      }
      break;
    }
    case TEXTURE_ID: {
      auto texture_id = static_cast<uint32_t*>(data);
      if (texture_id_data.empty() || texture_id_data.size() <= instance_index) {
        // XWARN("添加纹理id数据");
        texture_id_data.push_back(*texture_id);
        synchronize_update_mark(instance_index);
      } else {
        if (*texture_id != texture_id_data.at(instance_index)) {
          // 贴图id数据发生变化
          // 同步更新位置标记
          synchronize_update_mark(instance_index);
          // 更新此实例贴图id数据
          texture_id_data[instance_index] = *texture_id;
        }
      }
      break;
    }
    case FILL_COLOR: {
      auto fill_color = static_cast<QVector4D*>(data);
      if (fill_color_data.empty() || fill_color_data.size() <= instance_index) {
        // XWARN("添加填充颜色数据");
        fill_color_data.push_back(*fill_color);
        synchronize_update_mark(instance_index);
      } else {
        if (*fill_color != fill_color_data.at(instance_index)) {
          // 填充颜色数据发生变化
          // 同步更新位置标记
          synchronize_update_mark(instance_index);
          // 更新此实例填充颜色数据
          fill_color_data[instance_index] = *fill_color;
        }
      }
      break;
    }
  }
}

// 同步更新位置标记
void StaticRenderer::synchronize_update_mark(size_t instance_index) {
  // 同步更新标记
  if (update_list.empty()) {
    // 空,创建新的更新标记
    update_list.emplace_back(instance_index, 1);
  } else {
    // 非空,检查是否连续上一更新标记
    if (update_list.back().first + update_list.back().second ==
        instance_index) {
      // 连续--增加更新数量
      update_list.back().second++;
    } else {
      // 不连续,创建新的更新标记
      update_list.emplace_back(instance_index, 1);
    }
  }
}

// 更新gpu数据
void StaticRenderer::update_gpu_memory() {
  // 绑定实例缓冲区
  GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, instanceBO));

  for (const auto& [instance_start_index, instance_count] : update_list) {
    // 构建内存块
    std::vector<float> memory_block(instance_count * 11);
    for (int i = instance_start_index;
         i < instance_start_index + instance_count; i++) {
      //// 图形位置数据
      memory_block[(i - instance_start_index) * 11] = position_data[i].x();
      memory_block[(i - instance_start_index) * 11 + 1] = position_data[i].y();
      //// 图形尺寸
      memory_block[(i - instance_start_index) * 11 + 2] = size_data[i].x();
      memory_block[(i - instance_start_index) * 11 + 3] = size_data[i].y();
      //// 旋转角度
      memory_block[(i - instance_start_index) * 11 + 4] = rotation_data[i];
      //// 贴图方式
      memory_block[(i - instance_start_index) * 11 + 5] =
          texture_policy_data[i];
      //// 贴图id
      memory_block[(i - instance_start_index) * 11 + 6] = texture_id_data[i];
      //// 填充颜色
      memory_block[(i - instance_start_index) * 11 + 7] =
          fill_color_data[i].x();
      memory_block[(i - instance_start_index) * 11 + 8] =
          fill_color_data[i].y();
      memory_block[(i - instance_start_index) * 11 + 9] =
          fill_color_data[i].z();
      memory_block[(i - instance_start_index) * 11 + 10] =
          fill_color_data[i].w();
    }
    // 上传内存块到显存
    GLCALL(cvs->glBufferSubData(
        GL_ARRAY_BUFFER, (int)(instance_start_index * 11 * sizeof(float)),
        memory_block.size() * sizeof(float), memory_block.data()));
  }
}
// 重置更新标记
void StaticRenderer::reset_update() { update_list.clear(); }
