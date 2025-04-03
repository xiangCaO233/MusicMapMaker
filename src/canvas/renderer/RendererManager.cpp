#include "RendererManager.h"

RendererManager::RendererManager(GLCanvas* canvas, int oval_segment,
                                 int max_shape_count_per_renderer) {
  // 初始化渲染器
  static_renderer = std::make_shared<StaticRenderer>(
      canvas, oval_segment, max_shape_count_per_renderer);
  dynamic_renderer = std::make_shared<DynamicRenderer>(
      canvas, oval_segment, max_shape_count_per_renderer);
}

RendererManager::~RendererManager() {}

// 确定渲染内容
void RendererManager::finalize() {
  if (command_list_temp.empty()) {
    // 克隆指令列表到缓存
    command_list_temp = command_list;
    // 直接上传数据到gpu

  } else {
    // 逐一检查缓存队列指令变化情况,分析变化情况
    while (!command_list_temp.empty()) {
      if (command_list_temp.front() != command_list.front()) {
        auto& different_command = command_list.front();
        // 检查变化的具体内容
        if (different_command.is_volatile !=
            command_list_temp.front().is_volatile) {
          // 图元易变性发生变化,转移渲染器
        }
      }
    }
  }
}

// 添加矩形
void RendererManager::addRect(const QRectF& rect, TextureInfo* texture_info,
                              const QColor& fill_color, bool is_volatile) {
  // 在队尾直接生成渲染指令
  command_list.emplace_back(is_volatile, ShapeType::QUAD, rect, fill_color,
                            texture_info);
}
// 添加椭圆
void RendererManager::addEllipse(const QRectF& bounds,
                                 TextureInfo* texture_info,
                                 const QColor& fill_color, bool is_volatile) {
  // 在队尾直接生成渲染指令
  command_list.emplace_back(is_volatile, ShapeType::QUAD, bounds, fill_color,
                            texture_info);
}

// 渲染全部图形
void RendererManager::renderAll() {
  // 分析渲染指令,更新gpu数据
  finalize();

  // 按照队列顺序执行绘制
  while (!operation_queue.empty()) {
    auto& operetion = operation_queue.front();
    operetion.renderer->render(operetion.start_shape_index,
                               operetion.render_shape_count);
    operation_queue.pop();
  }

  // 重置渲染指令列表
  command_list.clear();
}

// 设置uniform浮点
void RendererManager::set_uniform_float(const char* location_name,
                                        float value) {
  static_renderer->bind();
  static_renderer->set_uniform_float(location_name, value);
  static_renderer->unbind();

  dynamic_renderer->bind();
  dynamic_renderer->set_uniform_float(location_name, value);
  dynamic_renderer->unbind();
}

// 设置uniform矩阵(4x4)
void RendererManager::set_uniform_mat4(const char* location_name,
                                       QMatrix4x4& mat) {
  static_renderer->bind();
  static_renderer->set_uniform_mat4(location_name, mat);
  static_renderer->unbind();

  dynamic_renderer->bind();
  dynamic_renderer->set_uniform_mat4(location_name, mat);
  dynamic_renderer->unbind();
}
