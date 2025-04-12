#include "StaticRenderer.h"

#include <QFile>
#include <cstdint>
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

StaticRenderer::StaticRenderer(GLCanvas* canvas,
                               std::shared_ptr<Shader> general_shader,
                               int oval_segment, int max_shape_count)
    : AbstractRenderer(canvas, general_shader, oval_segment, max_shape_count) {
  // 初始化实例缓冲区
  GLCALL(cvs->glGenBuffers(1, &instanceBO));
  // 图形位置2f,图形尺寸2f,旋转角度1f,图形贴图uv2f,贴图方式1f,贴图id1f,填充颜色4f
  GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, instanceBO));

  // 数据描述步长
  GLsizei stride = 12 * sizeof(float);

  // 位置信息
  // 描述location2 顶点缓冲0~1float为float类型数据--位置信息(用vec2接收)
  GLCALL(cvs->glEnableVertexAttribArray(2));
  GLCALL(cvs->glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, nullptr));
  // 每个实例变化一次
  GLCALL(cvs->glVertexAttribDivisor(2, 1));

  // 尺寸信息
  // 描述location3 顶点缓冲2~3float为float类型数据--尺寸信息(用vec2接收)
  GLCALL(cvs->glEnableVertexAttribArray(3));
  GLCALL(cvs->glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride,
                                    (void*)(2 * sizeof(float))));
  GLCALL(cvs->glVertexAttribDivisor(3, 1));

  // 旋转角度
  // 描述location4 顶点缓冲4~4float为float类型数据--旋转角度(用float接收)
  GLCALL(cvs->glEnableVertexAttribArray(4));
  GLCALL(cvs->glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, stride,
                                    (void*)(4 * sizeof(float))));
  GLCALL(cvs->glVertexAttribDivisor(4, 1));

  // 贴图uv方式
  // 描述location5 顶点缓冲5~5float为float类型数据--贴图uv方式(用float接收)
  GLCALL(cvs->glEnableVertexAttribArray(5));
  GLCALL(cvs->glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, stride,
                                    (void*)(5 * sizeof(float))));
  GLCALL(cvs->glVertexAttribDivisor(5, 1));

  // 贴图id信息
  // 描述location6 顶点缓冲6~6float为float类型数据--贴图id信息(用float接收)
  GLCALL(cvs->glEnableVertexAttribArray(6));
  GLCALL(cvs->glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, stride,
                                    (void*)(6 * sizeof(float))));
  GLCALL(cvs->glVertexAttribDivisor(6, 1));

  // 填充颜色信息
  // 描述location7 顶点缓冲7~10float为float类型数据--填充颜色信息(用vec4接收)
  GLCALL(cvs->glEnableVertexAttribArray(7));
  GLCALL(cvs->glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, stride,
                                    (void*)(7 * sizeof(float))));
  GLCALL(cvs->glVertexAttribDivisor(7, 1));

  // 圆角半径信息
  // 描述location8 顶点缓冲11~11float为float类型数据--圆角半径信息(用float接收)
  GLCALL(cvs->glEnableVertexAttribArray(8));
  GLCALL(cvs->glVertexAttribPointer(8, 1, GL_FLOAT, GL_FALSE, stride,
                                    (void*)(11 * sizeof(float))));
  GLCALL(cvs->glVertexAttribDivisor(8, 1));

  GLCALL(cvs->glBufferData(GL_ARRAY_BUFFER, (max_shape_count * stride), nullptr,
                           GL_STATIC_DRAW));
}

StaticRenderer::~StaticRenderer() {
  // 释放实例缓冲区
  GLCALL(cvs->glDeleteBuffers(1, &instanceBO));
}

// 同步数据
void StaticRenderer::synchronize_data(InstanceDataType data_type,
                                      size_t instance_index, void* data) {
  switch (data_type) {
    case InstanceDataType::POSITION: {
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
    case InstanceDataType::SIZE: {
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
    case InstanceDataType::ROTATION: {
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
    case InstanceDataType::TEXTURE_POLICY: {
      auto texture_policy = static_cast<uint32_t*>(data);
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
    case InstanceDataType::TEXTURE_ID: {
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
    case InstanceDataType::FILL_COLOR: {
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
    case InstanceDataType::RADIUS: {
      auto radius = static_cast<float*>(data);
      if (radius_data.empty() || radius_data.size() <= instance_index) {
        // XWARN("添加圆角半径数据");
        radius_data.push_back(*radius);
        synchronize_update_mark(instance_index);
      } else {
        if (*radius != radius_data.at(instance_index)) {
          // 圆角半径数据发生变化
          // 同步更新位置标记
          synchronize_update_mark(instance_index);
          // 更新此实例圆角半径数据
          radius_data[instance_index] = *radius;
        }
      }
      break;
    }
    default:
      break;
  }
}

// 同步更新位置标记
void StaticRenderer::synchronize_update_mark(size_t instance_index) {
  // 同步更新标记
  if (update_list.empty()) {
    // 空,创建新的更新标记
    update_list.emplace_back(instance_index, 1);
  } else {
    // 非空
    // 确保不是同一对象的不同属性变化
    if (update_list.back().first != instance_index) {
      // 检查是否连续上一更新标记
      if (update_list.back().first + update_list.back().second ==
          instance_index) {
        // 连续--增加更新数量
        update_list.back().second++;
      } else {
        if (update_list.back().first + update_list.back().second - 1 !=
            instance_index) {
          // 确保未记录过
          // 不连续,创建新的更新标记
          update_list.emplace_back(instance_index, 1);
        }
      }
    }
  }
}

// 更新gpu数据
void StaticRenderer::update_gpu_memory() {
  // 绑定实例缓冲区
  GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, instanceBO));

  for (const auto& [instance_start_index, instance_count] : update_list) {
    // 构建内存块
    std::vector<float> memory_block(instance_count * 12);
    for (int i = instance_start_index;
         i < instance_start_index + instance_count; i++) {
      //// 图形位置数据
      memory_block[(i - instance_start_index) * 12] = position_data[i].x();
      memory_block[(i - instance_start_index) * 12 + 1] = position_data[i].y();
      //// 图形尺寸
      memory_block[(i - instance_start_index) * 12 + 2] = size_data[i].x();
      memory_block[(i - instance_start_index) * 12 + 3] = size_data[i].y();
      //// 旋转角度
      memory_block[(i - instance_start_index) * 12 + 4] = rotation_data[i];
      //// 贴图方式
      memory_block[(i - instance_start_index) * 12 + 5] =
          texture_policy_data[i];
      //// 贴图id
      memory_block[(i - instance_start_index) * 12 + 6] = texture_id_data[i];
      //// 填充颜色
      memory_block[(i - instance_start_index) * 12 + 7] =
          fill_color_data[i].x();
      memory_block[(i - instance_start_index) * 12 + 8] =
          fill_color_data[i].y();
      memory_block[(i - instance_start_index) * 12 + 9] =
          fill_color_data[i].z();
      memory_block[(i - instance_start_index) * 12 + 10] =
          fill_color_data[i].w();
      // 圆角半径
      memory_block[(i - instance_start_index) * 12 + 11] = radius_data[i];
    }
    // 上传内存块到显存
    GLCALL(cvs->glBufferSubData(
        GL_ARRAY_BUFFER, (instance_start_index * 12 * sizeof(float)),
        memory_block.size() * sizeof(float), memory_block.data()));
  }
}
// 重置更新标记
void StaticRenderer::reset_update() { update_list.clear(); }
