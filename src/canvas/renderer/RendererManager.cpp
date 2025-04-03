#include "RendererManager.h"

RendererManager::RendererManager(QOpenGLFunctions* glf) {}

RendererManager::~RendererManager() {}

// 重置渲染指令队列
void RendererManager::reset() {}

// 确定渲染内容
void RendererManager::finalize() {
  while (!command_list_temp.empty()) {
    // 逐一检查缓存队列指令
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

// 添加矩形
void RendererManager::addRect(const QRectF& rect, TextureInfo* texture_info,
                              const QColor& fill_color, bool is_volatile) {
  // 生成渲染指令
  RenderCommand command;
  command.is_volatile = is_volatile;
  command.instance_shape = ShapeType::QUAD;
  command.texture_info = texture_info;
  command.instace_position = rect;
  command.fill_color = fill_color;
}
// 添加椭圆
void RendererManager::addEllipse(const QRectF& bounds,
                                 TextureInfo* texture_info,
                                 const QColor& fill_color, bool is_volatile) {
  // 生成渲染指令
  RenderCommand command;
  command.is_volatile = is_volatile;
  command.instance_shape = ShapeType::OVAL;
  command.texture_info = texture_info;
  command.instace_position = bounds;
  command.fill_color = fill_color;
}
