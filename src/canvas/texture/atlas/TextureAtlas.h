#ifndef TEXTURE_ATLAS_H
#define TEXTURE_ATLAS_H

#include "MaxRectsBinPack.h"
#include "texture/BaseTexturePool.h"

class TextureAtlas : public BaseTexturePool {
  MaxRectsBinPack packer;

 public:
  // 构造TextureAtlas
  TextureAtlas();
  // 析构TextureAtlas
  ~TextureAtlas() override;

  // 判满
  bool is_full() override;
  // 载入纹理
  // resource_path使用qrc中的路径
  void load_texture(const char* resource_path) override;
  // 生成纹理集
  void creat_atlas();
};

#endif  // TEXTURE_ATLAS_H
