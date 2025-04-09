#include "TexturePool.h"

#include <memory>
#include <string>
#include <utility>

#include "../../GLCanvas.h"
#include "colorful-log.h"
#include "texture/pool/BaseTexturePool.h"

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

// 最大支持连续的采样器数
uint32_t TexturePool::max_sampler_consecutive_count = 0;

// 最大支持连续总采样器数
uint32_t TexturePool::max_total_sampler_count = 0;

TexturePool::TexturePool(GLCanvas* canvas, bool dynamic_switch)
    : BaseTexturePool(canvas), dynamic_switch_texture(dynamic_switch) {
  XINFO("构造独立纹理存储纹理池");
  pool_type = TexturePoolType::BASE_POOL;
  // 可初始化的最大纹理批数
  texture_dozens.reserve(dynamic_switch ? max_total_sampler_count /
                                              max_sampler_consecutive_count
                                        : 1);
  // 预分配采样器数组
  for (auto& texture_dozen : texture_dozens) {
    texture_dozen.reserve(max_sampler_consecutive_count);
  }
}

TexturePool::~TexturePool() {
  for (const auto& [texture, textureID] : glhandler_map) {
    XINFO("卸载纹理: " + texture->name + "");
    GLCALL(cvs->glDeleteTextures(1, &textureID));
  }
}

// 判满
bool TexturePool::is_full() {
  if (texture_dozens.size() <
      (dynamic_switch_texture
           ? max_total_sampler_count / max_sampler_consecutive_count
           : 1)) {
    return false;
  }
  for (const auto& texture_dozen : texture_dozens) {
    if (texture_dozen.size() < max_sampler_consecutive_count) {
      return false;
    }
  }
  return true;
}

// 载入纹理
int TexturePool::load_texture(std::shared_ptr<TextureInstace> texture) {
  texture_map.try_emplace(texture->name, texture);
  // 添加批信息
  if (texture_dozens.empty() ||
      texture_dozens.back().size() >= max_sampler_consecutive_count) {
    texture_dozens.emplace_back();
  }
  batch_mapping[texture] = {texture_dozens.size() - 1,
                            texture_dozens.back().size()};
  texture_dozens.back().emplace_back(texture);
  texture->texture_id = current_id_handle++;

  XINFO("添加纹理: " + texture->name + "成功");
  return SUCCESS;
}

// 完成纹理池构造
void TexturePool::finalize() {
  XINFO("完成处理独立存储纹理池");
  for (auto& [texture, pair] : batch_mapping) {
    XINFO(texture->name + "位于[" + std::to_string(pair.first) + "]批次[" +
          std::to_string(pair.second) + "]索引处");
  }
  int dozen_index{0};
  for (auto& texture_dozen : texture_dozens) {
    XINFO("[" + std::to_string(dozen_index) + "]批:");
    for (const auto& texture : texture_dozen) {
      // 生成纹理ID
      GLCALL(cvs->glGenTextures(1, &glhandler_map[texture]));
      GLCALL(cvs->glBindTexture(GL_TEXTURE_2D, glhandler_map[texture]));

      // 设置纹理参数
      GLCALL(cvs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0));
      GLCALL(cvs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0));
      GLCALL(cvs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
      GLCALL(cvs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
      GLCALL(cvs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                  GL_NEAREST));
      GLCALL(cvs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                                  GL_NEAREST));

      // 上传纹理数据
      GLCALL(cvs->glTexImage2D(
          GL_TEXTURE_2D, 0, GL_RGBA, texture->texture_image.width(),
          texture->texture_image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE,
          texture->texture_image.bits()));

      XINFO("[" + std::to_string(texture->texture_id) + "]:" + texture->name +
            "已上传到gpu");
    }
    dozen_index++;
  }
}

// 使用此纹理池
// Base需使用指定批次
// Array不需要
void TexturePool::use(std::shared_ptr<BaseTexturePool> pool_reference,
                      std::shared_ptr<AbstractRenderer> renderer_context,
                      size_t batch_index) {
  // 更新使用纹理池方式
  renderer_context->set_uniform_integer(
      "texture_pool_usage", static_cast<int>(TexturePoolType::BASE_POOL));
  auto markit = temp_renderer_pool_mark.find(renderer_context);
  bool need_update{false};
  if (markit == temp_renderer_pool_mark.end()) {
    // 不存在此渲染器池标
    // 添加标志
    temp_renderer_pool_mark.try_emplace(renderer_context, batch_index);
    need_update = true;
  } else if (renderer_context->current_use_pool != pool_reference ||
             markit->second != batch_index)
    need_update = true;
  if (need_update) {
    // 修改渲染器当前正在使用的纹理池
    renderer_context->current_use_pool = pool_reference;
    // 更新指定使用批次的纹理单元
    for (int i = 0; i < texture_dozens[batch_index].size(); i++) {
      // 激活纹理单元
      GLCALL(cvs->glActiveTexture(GL_TEXTURE0 + i));
      XWARN("激活纹理单元:" + std::to_string(i));
      const auto& texture = texture_dozens[batch_index][i];
      // 绑定纹理句柄
      GLCALL(cvs->glBindTexture(GL_TEXTURE_2D, glhandler_map[texture]));
      XWARN("绑定纹理:" + texture->name +
            "->gl句柄:" + std::to_string(glhandler_map[texture]) +
            "->绑定到gl纹理单元:[" + std::to_string(i) + "]");
      // 更新uniform
      auto location_str = "samplers[" + std::to_string(i) + "]";
      auto cstr = location_str.c_str();
      renderer_context->set_sampler(cstr, i);
    }
  }
}

// 取消使用此纹理池
void TexturePool::unuse(std::shared_ptr<BaseTexturePool> pool_reference,
                        std::shared_ptr<AbstractRenderer> renderer_context) {
  // 更新使用纹理池方式
  renderer_context->set_uniform_integer("texture_pool_usage", 0);
  // 确认当前使用的是此纹理池
  bool need_update{false};
  if (renderer_context->current_use_pool == pool_reference) need_update = true;
  // 确认是才更新uniform
  if (need_update) {
    // 更新指定使用批次的纹理单元
    for (int i = 0; i < max_sampler_consecutive_count; i++) {
      // 更新uniform
      auto location_str = "samplers[" + std::to_string(i) + "]";
      auto cstr = location_str.c_str();
      // 清除采样器活跃
      renderer_context->set_sampler(cstr, -1);
    }
  }
}
