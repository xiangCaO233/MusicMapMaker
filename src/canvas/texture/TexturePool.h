#ifndef TEXTURE_POOL_H
#define TEXTURE_POOL_H

#include <vector>

#include "texture/BaseTexturePool.h"

class TexturePool : public BaseTexturePool {
  friend class RendererManager;

  // 全部纹理批
  std::vector<std::vector<TextureInstace*>> texture_dozens;

 public:
  // 构造TexturePool
  TexturePool();

  // 析构TexturePool
  ~TexturePool() override;

  // 最大支持连续的采样器数
  static uint32_t max_sampler_consecutive_count;

  // 最大支持连续总采样器数
  static uint32_t max_total_sampler_count;

  // 判满
  bool is_full() override;

  // 载入纹理
  // resource_path使用qrc中的路径
  void load_texture(const char* resource_path) override;

 protected:
};

#endif  // TEXTURE_POOL_H
