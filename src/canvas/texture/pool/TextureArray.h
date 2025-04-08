#ifndef TEXTURE_ARRAY_H
#define TEXTURE_ARRAY_H

#include <qsize.h>

#include "BaseTexturePool.h"
#define EXPECTED_SIZE 0x02

class TextureArray : public BaseTexturePool {
  QSize texture_size;
  friend class RendererManager;

 public:
  // 构造TextureArray-opengl使用纹理数组必须相同分辨率
  TextureArray(GLCanvas* canvas, QSize size);
  // 析构TextureArray
  ~TextureArray() override;

  // 最大采样器层数
  static uint32_t max_texture_layer;

  // 判满
  bool is_full() override;

  // 载入纹理
  int load_texture(std::shared_ptr<TextureInstace> texture) override;

  // 完成纹理池构造
  void finalize() override;

  // 使用纹理数组池
  void use();

 protected:
};

#endif  // TEXTURE_ARRAY_H
