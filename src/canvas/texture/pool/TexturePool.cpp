#include "TexturePool.h"

#include <memory>
#include <utility>

#include "colorful-log.h"

// 最大支持连续的采样器数
uint32_t TexturePool::max_sampler_consecutive_count = 0;

// 最大支持连续总采样器数
uint32_t TexturePool::max_total_sampler_count = 0;

TexturePool::TexturePool(GLCanvas* canvas, bool dynamic_switch)
    : BaseTexturePool(canvas) {
  XINFO("构造独立纹理存储纹理池");
  pool_type = TexturePoolType::BASE_POOL;
  // 可初始化的最大纹理批数
  texture_dozens.resize(dynamic_switch ? max_total_sampler_count /
                                             max_sampler_consecutive_count
                                       : 1);
  // 预分配采样器数组
  for (auto& texture_dozen : texture_dozens) {
    texture_dozen.reserve(max_sampler_consecutive_count);
  }
}

TexturePool::~TexturePool() {}

// 判满
bool TexturePool::is_full() {
  for (const auto& texture_dozen : texture_dozens) {
    if (texture_dozen.size() < max_sampler_consecutive_count) {
      return false;
    }
  }
  return true;
}

// 载入纹理
int TexturePool::load_texture(std::shared_ptr<TextureInstace> texture) {
  // 生成纹理信息
  auto info = std::make_shared<TextureInfo>();
  info->pool_ptr = this;
  info->texture_instance = texture;
  texture_map.try_emplace(texture->name, info);
  texture->texture_id = current_id_handle++;

  // 添加批信息
  if (texture_dozens.empty() ||
      texture_dozens.back().size() >= max_sampler_consecutive_count) {
    texture_dozens.emplace_back();
  }
  batch_mapping[info] = {texture_dozens.size() - 1,
                         texture_dozens.back().size()};
  texture_dozens.back().emplace_back(texture);

  XINFO("添加纹理: " + texture->name + "成功");
  return SUCCESS;
}

// 完成纹理池构造
void TexturePool::finalize() {
  XINFO("完成处理独立存储纹理池");
  // TODO(xiang 2025-04-07): 实现纹理创建和上传
}

// 使用指定纹理
// 需设置连续的uniform
void TexturePool::use_texture(std::shared_ptr<TextureInstace> texture) {}
