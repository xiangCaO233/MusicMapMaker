#ifndef TEXTURE_ARRAY_H
#define TEXTURE_ARRAY_H

#include "../BaseTexturePool.h"

class TextureArray : public BaseTexturePool {
 public:
  // 构造TextureArray
  TextureArray();
  // 析构TextureArray
  ~TextureArray() override;

  // 判满
  bool is_full() override;
  // 载入纹理
  // resource_path使用qrc中的路径
  void load_texture(const char* resource_path) override;

 protected:
};

#endif  // TEXTURE_ARRAY_H
