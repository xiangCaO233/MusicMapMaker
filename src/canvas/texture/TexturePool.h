#ifndef TEXTURE_POOL_H
#define TEXTURE_POOL_H

#include <qimage.h>

#include <cstdint>

struct TextureInstace {
  int32_t id;
  QImage texture_image;
  QRect position_in_atlas;
};

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
