#include "RendererManager.h"

#include <qvectornd.h>

#include <cstdint>
#include <memory>

#include "../../log/colorful-log.h"
#include "../texture/pool/BaseTexturePool.h"
#include "../texture/pool/TextureArray.h"
#include "../texture/pool/TexturePool.h"
#include "AbstractRenderer.h"
#include "dynamic/DynamicRenderer.h"
#include "font/FontRenderer.h"
#include "static/StaticRenderer.h"

uint32_t RendererManager::static_instance_index = 0;
uint32_t RendererManager::dynamic_instance_index = 0;
uint32_t RendererManager::fontr_instance_index = 0;

RendererManager::RendererManager(GLCanvas* canvas, int oval_segment,
                                 int max_shape_count_per_renderer) {
  XINFO("初始化通用着色器");
  // 用`:/`前缀访问qrc文件
#ifdef __APPLE__
  general_shader =
      std::make_shared<Shader>(canvas, ":/glsl/macos/vertexshader.glsl.vert",
                               ":/glsl/macos/fragmentshader.glsl.frag");
#else
  general_shader =
      std::make_shared<Shader>(canvas, ":/glsl/vertexshader.glsl.vert",
                               ":/glsl/fragmentshader.glsl.frag");
#endif  //__APPLE__
  XINFO("初始化字体着色器");
  /*
   *<file>glsl/vertfontshader.glsl.vert</file>
   *<file>glsl/fragfontshader.glsl.frag</file>
   */
  font_shader =
      std::make_unique<Shader>(canvas, ":glsl/vertfontshader.glsl.vert",
                               ":glsl/fragfontshader.glsl.frag");
  // 初始化渲染器
  static_renderer = std::make_shared<StaticRenderer>(
      canvas, general_shader, oval_segment, max_shape_count_per_renderer);
  dynamic_renderer = std::make_shared<DynamicRenderer>(
      canvas, general_shader, oval_segment, max_shape_count_per_renderer);
  font_renderer = std::make_shared<FontRenderer>(canvas, font_shader);
  font_renderer->load_font(
      "../resources/font/ComicShannsMonoNerdFont_Bold.otf");
  font_renderer->generate_ascii_buffer("ComicShannsMono Nerd Font");
  font_renderer->load_font(
      "../resources/font/QingNiaoHuaGuangJianMeiHei-2.ttf");
  font_renderer->generate_cjk_buffer("JMH");
}

RendererManager::~RendererManager() {}

// 确定渲染内容
void RendererManager::finalize() {
  if (command_list_temp.empty()) {
    // 克隆指令列表到缓存
    command_list_temp = command_list;
    // 遍历指令列表,直接同步渲染器的数据
    for (auto& command : command_list) {
      // 同步数据
      // 区分渲染器
      std::shared_ptr<AbstractRenderer> renderer =
          (command.is_text
               ? std::dynamic_pointer_cast<AbstractRenderer>(font_renderer)
               : (command.is_volatile
                      ? std::dynamic_pointer_cast<AbstractRenderer>(
                            dynamic_renderer)
                      : std::dynamic_pointer_cast<AbstractRenderer>(
                            static_renderer)));
      if (command.is_text) {
        // TODO(xiang 2025-04-12): 检查可合并的渲染文本指令
        if (  // 操作队列为空
            operation_queue.empty() ||
            // 操作队列尾渲染器不同
            operation_queue.back().renderer != renderer ||
            // 操作队列尾渲染图形类型不同
            operation_queue.back().shape_type != command.instance_shape) {
          // 新建渲染操作
          operation_queue.emplace(command, command.instance_shape, renderer,
                                  nullptr, fontr_instance_index, 1);
        } else {
          // 与队尾渲染操作可合并
          operation_queue.back().render_shape_count++;
        }
      } else {
        // 获取贴图
        const auto& texture = command.texture;

        // 更新渲染操作队列
        if (  // 操作队列为空
            operation_queue.empty() ||
            // 操作队列尾渲染器不同
            operation_queue.back().renderer != renderer ||
            // 操作队列尾渲染图形类型不同
            operation_queue.back().shape_type != command.instance_shape ||
            // 当前指令纹理池使用与操作队列尾不同时无纹理池使用
            (texture == nullptr) !=
                (operation_queue.back().texture_pool == nullptr) ||
            // 操作队列尾使用纹理池不同
            (texture ?
                     // texture非空,操作队列尾纹理池也非空
                 operation_queue.back().texture_pool != texture->pool_reference
                     // texture空,操作队列尾纹理池也空-可合并了
                     : false)) {
          // 新建渲染操作
          operation_queue.emplace(command, command.instance_shape, renderer,
                                  texture ? texture->pool_reference : nullptr,
                                  (command.is_volatile ? dynamic_instance_index
                                                       : static_instance_index),
                                  1);
        } else {
          // 与队尾渲染操作可合并
          // XWARN("合并操作");
          operation_queue.back().render_shape_count++;
        }
      }
      // 同步渲染指令到渲染器
      sync_renderer(renderer, command);
      // 更新渲染实例索引
      (command.is_text ? fontr_instance_index++
                       : (command.is_volatile ? dynamic_instance_index++
                                              : static_instance_index++));
    }
  } else {
    // 逐一检查缓存列表指令变化情况,分析变化情况
    // 遍历指令列表
    bool should_update_temp{false};
    if (command_list.size() < command_list_temp.size())
      should_update_temp = true;
    for (int i = 0; i < command_list.size(); i++) {
      // 同步数据
      // 取出指令
      auto& command = command_list[i];
      std::shared_ptr<AbstractRenderer> renderer =
          (command.is_text
               ? std::dynamic_pointer_cast<AbstractRenderer>(font_renderer)
               : (command.is_volatile
                      ? std::dynamic_pointer_cast<AbstractRenderer>(
                            dynamic_renderer)
                      : std::dynamic_pointer_cast<AbstractRenderer>(
                            static_renderer)));
      if (i < command_list_temp.size()) {
        // 检查缓存
        if (command != command_list_temp[i]) {
          // 渲染指令有变化
          // 区分渲染器
          should_update_temp = true;
          //  同步渲染指令到渲染器
          sync_renderer(renderer, command);
        }
      }
      if (command.is_text) {
        // TODO(xiang 2025-04-12): 检查可合并的渲染文本指令
        if (  // 操作队列为空
            operation_queue.empty() ||
            // 操作队列尾渲染器不同
            operation_queue.back().renderer != renderer ||
            // 操作队列尾渲染图形类型不同
            operation_queue.back().shape_type != command.instance_shape) {
          // 新建渲染操作
          operation_queue.emplace(command, command.instance_shape, renderer,
                                  nullptr, fontr_instance_index, 1);
        } else {
          // 与队尾渲染操作可合并
          operation_queue.back().render_shape_count++;
        }
      } else {
        // 获取贴图
        const auto& texture = command.texture;
        // 更新渲染操作队列
        if (  // 操作队列为空
            operation_queue.empty() ||
            // 操作队列尾渲染器不同
            operation_queue.back().renderer != renderer ||
            // 操作队列尾渲染图形类型不同
            operation_queue.back().shape_type != command.instance_shape ||
            // 当前指令纹理池使用与操作队列尾不同时无纹理池使用
            (texture == nullptr) !=
                (operation_queue.back().texture_pool == nullptr) ||
            // 操作队列尾使用纹理池不同
            (texture ?
                     // texture非空,操作队列尾纹理池也非空
                 operation_queue.back().texture_pool != texture->pool_reference
                     // texture空,操作队列尾纹理池也空-可合并了
                     : false)) {
          // 新建渲染操作
          operation_queue.emplace(command, command.instance_shape, renderer,
                                  texture ? texture->pool_reference : nullptr,
                                  (command.is_volatile ? dynamic_instance_index
                                                       : static_instance_index),
                                  1);
        } else {
          // 与队尾渲染操作可合并
          operation_queue.back().render_shape_count++;
        }
      }

      // 更新渲染实例索引
      (command.is_text ? fontr_instance_index++
                       : (command.is_volatile ? dynamic_instance_index++
                                              : static_instance_index++));
    }
    // 更新指令缓存
    if (should_update_temp) {
      command_list_temp = command_list;
    }
  }
  static_renderer->update_gpu_memory();
  dynamic_renderer->update_gpu_memory();
  font_renderer->update_gpu_memory();
  static_instance_index = 0;
  dynamic_instance_index = 0;
  fontr_instance_index = 0;
}

// 同步渲染指令到渲染器
void RendererManager::sync_renderer(
    const std::shared_ptr<AbstractRenderer>& renderer,
    RenderCommand& command) const {
  // 同步位置数据
  QVector2D position(command.instace_bound.x(), command.instace_bound.y());

  renderer->synchronize_data(
      InstanceDataType::POSITION,
      (command.is_text ? fontr_instance_index
                       : (command.is_volatile ? dynamic_instance_index
                                              : static_instance_index)),
      &position);

  // 同步角度数据
  renderer->synchronize_data(
      InstanceDataType::ROTATION,
      (command.is_text ? fontr_instance_index
                       : (command.is_volatile ? dynamic_instance_index
                                              : static_instance_index)),
      &command.rotation);

  // 同步填充颜色数据
  QVector4D fill_color(command.fill_color.red(), command.fill_color.green(),
                       command.fill_color.blue(), command.fill_color.alpha());
  renderer->synchronize_data(
      InstanceDataType::FILL_COLOR,
      (command.is_text ? fontr_instance_index
                       : (command.is_volatile ? dynamic_instance_index
                                              : static_instance_index)),
      &fill_color);
  // 区分文本渲染指令
  if (command.is_text) {
    // 同步字符串数据
    // 内部同步字符uv集数据
    renderer->synchronize_data(InstanceDataType::TEXT, fontr_instance_index,
                               &command.text);
  } else {
    // 同步尺寸数据
    QVector2D size(command.instace_bound.width(),
                   command.instace_bound.height());
    renderer->synchronize_data(
        InstanceDataType::SIZE,
        (command.is_volatile ? dynamic_instance_index : static_instance_index),
        &size);
    // 获取贴图
    const auto& texture = command.texture;

    // 同步贴图方式数据
    uint32_t policy = 0;
    if (texture) {
      policy = (static_cast<uint32_t>(command.texture_effect) |
                static_cast<uint32_t>(command.texture_complementmode) |
                static_cast<uint32_t>(command.texture_fillmode)) |
               (static_cast<uint32_t>(command.texture_alignmode));
    }
    renderer->synchronize_data(
        InstanceDataType::TEXTURE_POLICY,
        (command.is_volatile ? dynamic_instance_index : static_instance_index),
        &policy);

    // 同步贴图id数据
    int texture_id = 0;
    if (texture) {
      texture_id = texture->texture_id;
    }
    renderer->synchronize_data(
        InstanceDataType::TEXTURE_ID,
        (command.is_volatile ? dynamic_instance_index : static_instance_index),
        &texture_id);
    // 同步圆角半径数据
    renderer->synchronize_data(
        InstanceDataType::RADIUS,
        (command.is_volatile ? dynamic_instance_index : static_instance_index),
        &command.radius);
  }
}

// 添加文本
void RendererManager::addText(const QPointF& pos, std::u32string& text,
                              float font_size, std::string font_family,
                              const QColor& fill_color, float rotation) {
  uint32_t xoffset = 0;
  // 在队尾直接生成渲染指令
  for (int i = 0; i < text.size(); i++) {
    Text t(font_family, font_size, text.at(i), i == text.size() - 1);
    CharacterGlyph glyph;
    font_renderer->get_character_glyph(t.font_family, t.font_size, t.character,
                                       glyph);
    // 逐字符生成渲染指令
    command_list.emplace_back(true, t, false, ShapeType::TEXT,
                              QRectF(pos.x() + xoffset, pos.y(), 0, 0),
                              rotation, fill_color, 0.0f, nullptr,
                              texture_effect, texture_alignmode,
                              texture_fillmode, texture_complementmode);
    xoffset += glyph.xadvance / 64;
  }
}

// 添加矩形
void RendererManager::addRect(const QRectF& rect,
                              std::shared_ptr<TextureInstace> texture,
                              const QColor& fill_color, float rotation,
                              bool is_volatile) {
  // 在队尾直接生成渲染指令
  command_list.emplace_back(false, Text("", 0, U' ', false), is_volatile,
                            ShapeType::RECT, rect, rotation, fill_color, 0.0f,
                            texture, texture_effect, texture_alignmode,
                            texture_fillmode, texture_complementmode);
}

// 添加圆角矩形-radius > 1:启用边缘渐变
void RendererManager::addRoundRect(const QRectF& rect,
                                   std::shared_ptr<TextureInstace> texture,
                                   const QColor& fill_color, float rotation,
                                   float radius_ratios, bool is_volatile) {
  // 在队尾直接生成渲染指令
  command_list.emplace_back(
      false, Text("", 0, U' ', false), is_volatile, ShapeType::RECT, rect,
      rotation, fill_color, radius_ratios, texture, texture_effect,
      texture_alignmode, texture_fillmode, texture_complementmode);
}

// 添加椭圆
void RendererManager::addEllipse(const QRectF& bounds,
                                 std::shared_ptr<TextureInstace> texture,
                                 const QColor& fill_color, float rotation,
                                 bool is_volatile) {
  // 在队尾直接生成渲染指令
  command_list.emplace_back(false, Text("", 0, u' ', false), is_volatile,
                            ShapeType::OVAL, bounds, rotation, fill_color, 0.0f,
                            texture, texture_effect, texture_alignmode,
                            texture_fillmode, texture_complementmode);
}

// 渲染全部图形
void RendererManager::renderAll() {
  // 分析渲染指令,更新gpu数据
  finalize();

  // 按照队列顺序执行绘制
  while (!operation_queue.empty()) {
    auto& operetion = operation_queue.front();
    operetion.renderer->bind();
    if (operetion.shape_type == ShapeType::TEXT) {
      // 渲染文本
    } else {
      // 普通渲染图形
      // 检查纹理池
      auto& texpool = operetion.texture_pool;
      if (texpool) {
        auto base_pool = std::dynamic_pointer_cast<TexturePool>(texpool);
        if (base_pool) {
          // 使用basepool
          // XINFO("使用base pool");
          // 使用头指令纹理所处批次
          base_pool->use(
              base_pool, operetion.renderer,
              base_pool->batch_mapping[operetion.head_command.texture].first);
        } else {
          auto array_pool = std::dynamic_pointer_cast<TextureArray>(texpool);
          if (array_pool) {
            // 使用arraypool
            // XINFO("使用array pool");
            // 使用纹理数组池
            array_pool->use(array_pool, operetion.renderer);
          }
        }
      } else {
        // 不使用纹理池
        operetion.renderer->shader->set_uniform_integer("texture_pool_usage",
                                                        0);
      }
    }

    operetion.renderer->render(operetion.shape_type,
                               operetion.start_shape_index,
                               operetion.render_shape_count);
    operetion.renderer->unbind();
    operation_queue.pop();
  }

  // 重置渲染指令列表
  command_list.clear();

  // 重置渲染器更新标记
  static_renderer->reset_update();
  dynamic_renderer->reset_update();
  font_renderer->reset_update();
}

// 设置采样器
void RendererManager::set_general_sampler(const char* name, int value) const {
  general_shader->use();
  general_shader->set_sampler(name, value);
  general_shader->unuse();
}
void RendererManager::set_fontpool_sampler(const char* name, int value) const {
  font_shader->use();
  font_shader->set_sampler(name, value);
  font_shader->unuse();
}

// 设置uniform浮点
void RendererManager::set_general_uniform_float(const char* location_name,
                                                float value) const {
  general_shader->use();
  general_shader->set_uniform_float(location_name, value);
  general_shader->unuse();
}
void RendererManager::set_fontpool_uniform_float(const char* location_name,
                                                 float value) const {
  font_shader->use();
  font_shader->set_uniform_float(location_name, value);
  font_shader->unuse();
}

// 更新全部投影矩阵
void RendererManager::update_all_projection_mat(const char* location_name,
                                                QMatrix4x4& mat) {
  set_general_uniform_mat4(location_name, mat);
  set_fontpool_uniform_mat4(location_name, mat);
}

// 设置uniform矩阵(4x4)
void RendererManager::set_general_uniform_mat4(const char* location_name,
                                               QMatrix4x4& mat) const {
  general_shader->use();
  general_shader->set_uniform_mat4(location_name, mat);
  general_shader->unuse();
}
void RendererManager::set_fontpool_uniform_mat4(const char* location_name,
                                                QMatrix4x4& mat) const {
  font_shader->use();
  font_shader->set_uniform_mat4(location_name, mat);
  font_shader->unuse();
}
