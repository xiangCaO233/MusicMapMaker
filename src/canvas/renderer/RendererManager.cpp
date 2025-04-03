#include "RendererManager.h"

RendererManager::RendererManager(GLCanvas* canvas, int oval_segment,
                                 int max_shape_count_per_renderer) {
  // 初始化渲染器
  static_renderer = std::make_unique<StaticRenderer>(
      canvas, oval_segment, max_shape_count_per_renderer);
  dynamic_renderer = std::make_unique<DynamicRenderer>(
      canvas, oval_segment, max_shape_count_per_renderer);
}

RendererManager::~RendererManager() {}

// 重置渲染指令队列
void RendererManager::reset() {}

// 确定渲染内容
void RendererManager::finalize() {
  if (command_list_temp.empty()) {
    // 克隆到缓存
    command_list_temp = command_list;
  } else {
    while (!command_list_temp.empty()) {
      // 逐一检查缓存队列指令变化情况
      if (command_list_temp.front() != command_list.front()) {
        auto& different_command = command_list.front();
        // 检查变化的具体内容
        if (different_command.is_volatile !=
            command_list_temp.front().is_volatile) {
          // 图元易变性发生变化,转移渲染器
        }
      }
      command_list.pop();
      command_list_temp.pop();
    }
  }
}

// 添加矩形
void RendererManager::addRect(const QRectF& rect, TextureInfo* texture_info,
                              const QColor& fill_color, bool is_volatile) {
  // 在队尾直接生成渲染指令
  command_list.emplace(is_volatile, ShapeType::QUAD, rect, fill_color,
                       texture_info);
}
// 添加椭圆
void RendererManager::addEllipse(const QRectF& bounds,
                                 TextureInfo* texture_info,
                                 const QColor& fill_color, bool is_volatile) {
  // 在队尾直接生成渲染指令
  command_list.emplace(is_volatile, ShapeType::QUAD, bounds, fill_color,
                       texture_info);
}

// 渲染全部图形
void RendererManager::renderAll() {}
