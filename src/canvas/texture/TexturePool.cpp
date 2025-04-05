#include "TexturePool.h"
// 最大支持连续的采样器数
uint32_t TexturePool::max_sampler_consecutive_count = 0;

// 最大支持连续总采样器数
uint32_t TexturePool::max_total_sampler_count = 0;

TexturePool::TexturePool() {
  pool_type = TexturePoolType::BASE_POOL;
  // 可初始化的最大纹理批数
  texture_dozens.resize(max_total_sampler_count /
                        max_sampler_consecutive_count);
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
void TexturePool::load_texture(const char* resource_path) {}
