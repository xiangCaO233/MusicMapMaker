#include "TexturePool.h"

// 最大支持连续的采样器数
uint32_t TexturePool::max_sampler_consecutive_count = 0;

// 最大支持连续总采样器数
uint32_t TexturePool::max_total_sampler_count = 0;

TexturePool::TexturePool(GLCanvas* canvas, bool dynamic_switch)
    : BaseTexturePool(canvas) {
  pool_type = TexturePoolType::BASE_POOL;
  // 可初始化的最大纹理批数
  texture_dozens.resize(dynamic_switch ? max_total_sampler_count /
                                             max_sampler_consecutive_count
                                       : 1);
  // 预分配采样器数组
  for (auto& texture_dozen : texture_dozens) {
    texture_dozen.resize(max_sampler_consecutive_count);
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
  return SUCCESS;
}

// 完成纹理池构造
void TexturePool::finalize() {}

// 使用指定纹理
// 需设置连续的uniform
void TexturePool::use_texture(std::shared_ptr<TextureInstace> texture) {}
