#ifndef TEXTURE_POOL_H
#define TEXTURE_POOL_H

class TexturePool {
  // 使用纹理图集
  bool use_texture_atlas;

 public:
  // 构造TexturePool
  explicit TexturePool(bool is_use_texture_atlas);
  // 析构TexturePool
  virtual ~TexturePool();
};

#endif  // TEXTURE_POOL_H
