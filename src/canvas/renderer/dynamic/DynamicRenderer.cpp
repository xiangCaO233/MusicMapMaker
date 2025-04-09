#include "DynamicRenderer.h"

#include <QFile>
#include <vector>

#include "../../../log/colorful-log.h"
#include "../../GLCanvas.h"
#include "renderer/AbstractRenderer.h"

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

DynamicRenderer::DynamicRenderer(GLCanvas* canvas, int oval_segment,
                                 int max_shape_count)
    : AbstractRenderer(canvas, oval_segment, max_shape_count) {
  // 初始化实例缓冲区
  GLCALL(cvs->glGenBuffers(7, instanceBO));

  // [0] 图形位置,[1] 图形尺寸,[2] 旋转角度
  // [3] 图形贴图方式,[4] 贴图id,[5]填充颜色,[6]圆角半径

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
  GLCALL(cvs->glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(float),
                                    nullptr));
  GLCALL(cvs->glBufferData(GL_ARRAY_BUFFER,
                           (int)(max_shape_count * 1 * sizeof(float)), nullptr,
                           GL_DYNAMIC_DRAW));
  GLCALL(cvs->glVertexAttribDivisor(4, 1));

  // 贴图uv方式
  GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, instanceBO[3]));
  // 描述location5 顶点缓冲0~0float为float类型数据--贴图uv方式(用float接收)
  GLCALL(cvs->glEnableVertexAttribArray(5));
  GLCALL(cvs->glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(float),
                                    nullptr));
  GLCALL(cvs->glBufferData(GL_ARRAY_BUFFER,
                           (int)(max_shape_count * 1 * sizeof(float)), nullptr,
                           GL_DYNAMIC_DRAW));
  GLCALL(cvs->glVertexAttribDivisor(5, 1));

  // 贴图id信息
  GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, instanceBO[4]));
  // 描述location6 顶点缓冲0~0float为float类型数据--贴图id信息(用float接收)
  GLCALL(cvs->glEnableVertexAttribArray(6));
  GLCALL(cvs->glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(float),
                                    nullptr));
  GLCALL(cvs->glBufferData(GL_ARRAY_BUFFER,
                           (int)(max_shape_count * 1 * sizeof(float)), nullptr,
                           GL_DYNAMIC_DRAW));
  GLCALL(cvs->glVertexAttribDivisor(6, 1));

  // 填充颜色信息
  GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, instanceBO[5]));
  // 描述location7 顶点缓冲0~3float为float类型数据--填充颜色信息(用vec4接收)
  GLCALL(cvs->glEnableVertexAttribArray(7));
  GLCALL(cvs->glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                                    nullptr));
  GLCALL(cvs->glBufferData(GL_ARRAY_BUFFER,
                           (int)(max_shape_count * 4 * sizeof(float)), nullptr,
                           GL_DYNAMIC_DRAW));
  GLCALL(cvs->glVertexAttribDivisor(7, 1));

  // 圆角半径
  GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, instanceBO[6]));
  // 描述location8 顶点缓冲0~0float为float类型数据--圆角半径信息(用vec1接收)
  GLCALL(cvs->glEnableVertexAttribArray(8));
  GLCALL(cvs->glVertexAttribPointer(8, 1, GL_FLOAT, GL_FALSE, sizeof(float),
                                    nullptr));
  GLCALL(cvs->glBufferData(GL_ARRAY_BUFFER,
                           (int)(max_shape_count * sizeof(float)), nullptr,
                           GL_DYNAMIC_DRAW));
  GLCALL(cvs->glVertexAttribDivisor(8, 1));

  // 初始化着色器程序
  init_shader_programe();
}

DynamicRenderer::~DynamicRenderer() {}

// 初始化着色器程序
void DynamicRenderer::init_shader_programe() {
  auto vshader = GLCALL(cvs->glCreateShader(GL_VERTEX_SHADER));
  auto fshader = GLCALL(cvs->glCreateShader(GL_FRAGMENT_SHADER));

  // 用`:/`前缀访问qrc文件
#ifdef __APPLE__
  QFile vertfile(":/glsl/macos/vertexshader-dynamic.glsl.vert");
  QFile fragfile(":/glsl/macos/fragmentshader-dynamic.glsl.frag");
#else
  QFile vertfile(":/glsl/vertexshader-dynamic.glsl.vert");
  QFile fragfile(":/glsl/fragmentshader-dynamic.glsl.frag");
#endif  //__APPLE__

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

// 同步数据
void DynamicRenderer::synchronize_data(InstanceDataType data_type,
                                       size_t instance_index, void* data) {
  switch (data_type) {
    case POSITION: {
      auto pos = static_cast<QVector2D*>(data);
      if (position_data.empty() || position_data.size() <= instance_index) {
        // XWARN("添加位置数据");
        position_data.push_back(*pos);
        synchronize_update_mark(data_type, instance_index);
      } else {
        if (*pos != position_data.at(instance_index)) {
          // XWARN("位置数据更新");
          // 位置数据发生变化
          // 同步更新位置标记
          synchronize_update_mark(data_type, instance_index);
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
        synchronize_update_mark(data_type, instance_index);
      } else {
        if (*size != size_data.at(instance_index)) {
          // XWARN("尺寸更新");
          synchronize_update_mark(data_type, instance_index);
          size_data[instance_index] = *size;
        }
      }
      break;
    }
    case ROTATION: {
      auto rotation = static_cast<float*>(data);
      if (rotation_data.empty() || rotation_data.size() <= instance_index) {
        // XWARN("添加旋转数据");
        rotation_data.push_back(*rotation);
        synchronize_update_mark(data_type, instance_index);
      } else {
        if (*rotation != rotation_data.at(instance_index)) {
          // XWARN("旋转更新");
          synchronize_update_mark(data_type, instance_index);
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
        synchronize_update_mark(data_type, instance_index);
      } else {
        if (*texture_policy != texture_policy_data.at(instance_index)) {
          // XWARN("纹理填充策略更新");
          synchronize_update_mark(data_type, instance_index);
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
        synchronize_update_mark(data_type, instance_index);
      } else {
        if (*texture_id != texture_id_data.at(instance_index)) {
          // XWARN("纹理id更新");
          synchronize_update_mark(data_type, instance_index);
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
        synchronize_update_mark(data_type, instance_index);
      } else {
        if (*fill_color != fill_color_data.at(instance_index)) {
          // XWARN("填充颜色更新");
          synchronize_update_mark(data_type, instance_index);
          fill_color_data[instance_index] = *fill_color;
        }
      }
      break;
    }
    case RADIUS: {
      auto radius = static_cast<float*>(data);
      if (radius_data.empty() || radius_data.size() <= instance_index) {
        // XWARN("添加圆角半径数据");
        radius_data.push_back(*radius);
        synchronize_update_mark(data_type, instance_index);
      } else {
        if (*radius != radius_data.at(instance_index)) {
          // XWARN("圆角半径更新");
          synchronize_update_mark(data_type, instance_index);
          radius_data[instance_index] = *radius;
        }
      }
      break;
    }
  }
}

// 同步更新标记
void DynamicRenderer::synchronize_update_mark(InstanceDataType data_type,
                                              size_t instance_index) {
  std::vector<std::pair<size_t, uint32_t>>* mark_list;
  switch (data_type) {
    case POSITION: {
      auto listit = update_mapping.find(POSITION);
      if (listit == update_mapping.end()) {
        // 添加缓冲区映射并更新迭代器
        listit = update_mapping.try_emplace(POSITION).first;
      }
      // 更新连续更新标记映射
      mark_list = &listit->second;
      break;
    }
    case SIZE: {
      auto listit = update_mapping.find(SIZE);
      if (listit == update_mapping.end()) {
        // 添加缓冲区映射并更新迭代器
        listit = update_mapping.try_emplace(SIZE).first;
      }
      // 更新连续更新标记映射
      mark_list = &listit->second;
      break;
    }
    case ROTATION: {
      auto listit = update_mapping.find(ROTATION);
      if (listit == update_mapping.end()) {
        // 添加缓冲区映射并更新迭代器
        listit = update_mapping.try_emplace(ROTATION).first;
      }
      // 更新连续更新标记映射
      mark_list = &listit->second;
      break;
    }
    case TEXTURE_POLICY: {
      auto listit = update_mapping.find(TEXTURE_POLICY);
      if (listit == update_mapping.end()) {
        // 添加缓冲区映射并更新迭代器
        listit = update_mapping.try_emplace(TEXTURE_POLICY).first;
      }
      // 更新连续更新标记映射
      mark_list = &listit->second;
      break;
    }
    case TEXTURE_ID: {
      auto listit = update_mapping.find(TEXTURE_ID);
      if (listit == update_mapping.end()) {
        // 添加缓冲区映射并更新迭代器
        listit = update_mapping.try_emplace(TEXTURE_ID).first;
      }
      // 更新连续更新标记映射
      mark_list = &listit->second;
      break;
    }
    case FILL_COLOR: {
      auto listit = update_mapping.find(FILL_COLOR);
      if (listit == update_mapping.end()) {
        // 添加缓冲区映射并更新迭代器
        listit = update_mapping.try_emplace(FILL_COLOR).first;
      }
      // 更新连续更新标记映射
      mark_list = &listit->second;
      break;
    }
    case RADIUS: {
      auto listit = update_mapping.find(RADIUS);
      if (listit == update_mapping.end()) {
        // 添加缓冲区映射并更新迭代器
        listit = update_mapping.try_emplace(RADIUS).first;
      }
      // 更新连续更新标记映射
      mark_list = &listit->second;
      break;
    }
  }
  if (mark_list->empty()) {
    // 空,创建新的更新标记
    mark_list->emplace_back(instance_index, 1);
  } else {
    // 非空,检查是否连续上一更新标记
    if (mark_list->back().first + mark_list->back().second == instance_index) {
      // 连续--增加更新数量
      mark_list->back().second++;
    } else {
      // 不连续,创建新的更新标记
      mark_list->emplace_back(instance_index, 1);
    }
  }
}

// 更新gpu数据
void DynamicRenderer::update_gpu_memory() {
  // instanceBO
  // [0] 图形位置,[1] 图形尺寸,[2] 旋转角度
  // [3] 图形贴图方式,[4] 贴图id,[5]填充颜色
  for (const auto& [data_type, mark_map] : update_mapping) {
    // 绑定对应缓冲区
    uint32_t buffer;
    switch (data_type) {
      case POSITION: {
        buffer = instanceBO[0];
        break;
      }
      case SIZE: {
        buffer = instanceBO[1];
        break;
      }
      case ROTATION: {
        buffer = instanceBO[2];
        break;
      }
      case TEXTURE_POLICY: {
        buffer = instanceBO[3];
        break;
      }
      case TEXTURE_ID: {
        buffer = instanceBO[4];
        break;
      }
      case FILL_COLOR: {
        buffer = instanceBO[5];
        break;
      }
      case RADIUS: {
        buffer = instanceBO[6];
        break;
      }
    }
    GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, buffer));
    std::vector<float> memory_block;
    for (const auto& [instance_start_index, instance_count] : mark_map) {
      size_t memory_block_size;
      switch (data_type) {
        case POSITION: {
          // 每实例2float
          memory_block_size = 2 * instance_count;
          // 构造位置数据内存块
          memory_block.resize(memory_block_size);
          for (int i = instance_start_index;
               i < instance_start_index + instance_count; i++) {
            memory_block[2 * (i - instance_start_index)] = position_data[i].x();
            memory_block[2 * (i - instance_start_index) + 1] =
                position_data[i].y();
          }
          break;
        }
        case SIZE: {
          // 每实例2float
          memory_block_size = 2 * instance_count;
          // 构造尺寸数据内存块
          memory_block.resize(memory_block_size);
          for (int i = instance_start_index;
               i < instance_start_index + instance_count; i++) {
            memory_block[2 * (i - instance_start_index)] = size_data[i].x();
            memory_block[2 * (i - instance_start_index) + 1] = size_data[i].y();
          }
          break;
        }
        case ROTATION: {
          // 每实例1float
          memory_block_size = instance_count;
          // 构造旋转角度数据内存块
          memory_block.resize(memory_block_size);
          for (int i = instance_start_index;
               i < instance_start_index + instance_count; i++)
            memory_block[(i - instance_start_index)] = rotation_data[i];
          break;
        }
        case TEXTURE_POLICY: {
          // 每实例1float
          memory_block_size = instance_count;
          // 构造纹理填充策略数据内存块
          memory_block.resize(memory_block_size);
          for (int i = instance_start_index;
               i < instance_start_index + instance_count; i++)
            memory_block[(i - instance_start_index)] = texture_policy_data[i];
          break;
        }
        case TEXTURE_ID: {
          // 每实例1float
          memory_block_size = instance_count;
          // 构造纹理id数据内存块
          memory_block.resize(memory_block_size);
          for (int i = instance_start_index;
               i < instance_start_index + instance_count; i++)
            memory_block[(i - instance_start_index)] = texture_id_data[i];
          break;
        }
        case FILL_COLOR: {
          // 每实例4float
          memory_block_size = 4 * instance_count;
          // 构造填充颜色数据内存块
          memory_block.resize(memory_block_size);
          for (int i = instance_start_index;
               i < instance_start_index + instance_count; i++) {
            memory_block[4 * (i - instance_start_index)] =
                fill_color_data[i].x();
            memory_block[4 * (i - instance_start_index) + 1] =
                fill_color_data[i].y();
            memory_block[4 * (i - instance_start_index) + 2] =
                fill_color_data[i].z();
            memory_block[4 * (i - instance_start_index) + 3] =
                fill_color_data[i].w();
          }
          break;
        }
        case RADIUS: {
          // 每实例1float
          memory_block_size = instance_count;
          // 构造圆角半径数据内存块
          memory_block.resize(memory_block_size);
          for (int i = instance_start_index;
               i < instance_start_index + instance_count; i++)
            memory_block[(i - instance_start_index)] = radius_data[i];
          break;
        }
      }
      // 上传内存块到显存
      GLCALL(cvs->glBufferSubData(
          GL_ARRAY_BUFFER,
          (int)(instance_start_index * (memory_block_size / instance_count) *
                sizeof(float)),
          memory_block.size() * sizeof(float), memory_block.data()));
    }
  }
}

// 重置更新标记
void DynamicRenderer::reset_update() { update_mapping.clear(); }
