#ifndef TEXTURE_ATLAS_H
#define TEXTURE_ATLAS_H

#include "MaxRectsBinPack.h"
#include "texture/BaseTexturePool.h"

class TextureAtlas : public BaseTexturePool {
  // 打包器
  MaxRectsBinPack packer;

  // 是否完成打包
  bool is_packed;

  // 生成纹理集
  void creat_atlas();

  friend class RendererManager;

 public:
  // 构造TextureAtlas
  TextureAtlas(int width = 4096, int height = 4096, float expandrate = 1.05);
  // 析构TextureAtlas
  ~TextureAtlas() override;

  // 判满
  bool is_full() override;
  // 载入纹理
  // resource_path使用qrc中的路径
  void load_texture(const char* resource_path) override;
};

#endif  // TEXTURE_ATLAS_H
