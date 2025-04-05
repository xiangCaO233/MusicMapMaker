#ifndef TEXTURE_ARRAY_H
#define TEXTURE_ARRAY_H

#include <qsize.h>

#include "../BaseTexturePool.h"

class TextureArray : public BaseTexturePool {
  QSize texture_size;
  friend class RendererManager;

 public:
  // 构造TextureArray-opengl使用纹理数组必须相同分辨率
  TextureArray(QSize& size);
  // 析构TextureArray
  ~TextureArray() override;

  // 最大采样器层数
  static uint32_t max_texture_layer;

  // 判满
  bool is_full() override;
  // 载入纹理
  // resource_path使用qrc中的路径
  void load_texture(const char* resource_path) override;

 protected:
};

#endif  // TEXTURE_ARRAY_H
