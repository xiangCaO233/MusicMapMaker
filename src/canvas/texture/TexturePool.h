#ifndef TEXTURE_POOL_H
#define TEXTURE_POOL_H

#include "texture/BaseTexturePool.h"

class TexturePool : public BaseTexturePool {
 public:
  // 构造TexturePool
  TexturePool();
  // 析构TexturePool
  ~TexturePool() override;
  // 判满
  bool is_full() override;
  // 载入纹理
  // resource_path使用qrc中的路径
  void load_texture(const char* resource_path) override;

 protected:
};

#endif  // TEXTURE_POOL_H
